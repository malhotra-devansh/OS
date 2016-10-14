
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include "cs402.h"
#include "my402list.h"

#define MAX_VAL 2147483647
FILE *file_pointer = NULL;

struct timeval emulation_start_time, emulation_end_time;
struct timeval previous_packet_arrival_time;
pthread_mutex_t token_bucket_lock = PTHREAD_MUTEX_INITIALIZER;

int is_tracedriven = 0;
int token_serial_number = 0;
int bucket_token_count = 0;
int packet_serial_number = 0;
My402List queue_Q1, queue_Q2;

// Variables indicating termination of threads
int stop1 = 0;
int stop2 = 0;
int stop3 = 0;
int stop4 = 0;

 //Values required for statistics
int tokens_dropped = 0;
int Q1_packets_processed = 0;
int discarded_packets = 0;
int packets_completed = 0;
double packet_inter_arrival_time_sum = 0;
double packet_service_time_sum = 0;
double total_time_spent_in_Q1 = 0;
double total_time_spent_in_Q2 = 0;
double sum_packet_service_time_in_S1 = 0;
double sum_packet_service_time_in_S2 = 0;
double sum_time_packet_spent_in_system = 0;
double sum_of_time_in_system_square = 0;

// Condition variables for service threads
pthread_cond_t execute_service = PTHREAD_COND_INITIALIZER;

/* this is a reference to the first thread */
pthread_t token_bucket_thread;
/* this is a reference to the second thread */
pthread_t queue_Q1_thread;
/* this is a reference to the third thread */
pthread_t service_1;
/* this is a reference to the fourth thread */
pthread_t service_2;
/* this is a reference to the fifth thread */
pthread_t handler_thread;

sigset_t set;

static const struct option long_options[] = {
										{"lambda", required_argument, NULL, 'l'},
										{"mu", required_argument,NULL, 'm'},
										{"r", required_argument, NULL, 'r'},
										{"B", required_argument, NULL, 'B'},
										{"P", required_argument, NULL, 'P'},
										{"n", required_argument, NULL, 'n'},
										{"t", required_argument, NULL, 't'},
										{0,0,0,0}
									  };

typedef struct parameters
{
	int token_bucket_depth;
	double token_arrival_rate;
	double token_inter_arrival_time;
	double packet_arrival_rate;
	int tokens_required_by_packet;
	double service_rate;
	int num_of_packets;
	char* filename;
}params;

typedef struct packet
{
	int packet_id;
	double inter_arrival_time;
	double actual_inter_arrival_time;
	int tokens_required;
	double service_time;
	double actual_service_time;
	struct timeval arrival_time;
	struct timeval arrival_time_Q1;
	struct timeval departure_time_Q1;
	struct timeval arrival_time_Q2;
	struct timeval departure_time_Q2;
	struct timeval exit_time;
	double time_spent_in_Q1;
	double time_spent_in_Q2;
	double time_in_system;
}pack;

double format_time(struct timeval tv)
{
	double value = tv.tv_sec*1000000 + tv.tv_usec;
	return (value);
}

void move_from_Q1_to_Q2(My402ListElem* elem)
{
	//int isEmpty = 0;
	double packet_time;
	pack* temp_packet = (pack*)elem->obj;
	bucket_token_count -= temp_packet->tokens_required;
	// Removing from Q1
	My402ListUnlink(&queue_Q1, elem);
	gettimeofday(&(temp_packet->departure_time_Q1), NULL);
	packet_time = (format_time(temp_packet->departure_time_Q1) - format_time(emulation_start_time))/1000;

	// Time spent in queue Q1
	temp_packet->time_spent_in_Q1 = (format_time(temp_packet->departure_time_Q1) - format_time(temp_packet->arrival_time_Q1))/1000;

	Q1_packets_processed += 1;
	fprintf(stdout,"%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token%s\n", packet_time, temp_packet->packet_id, temp_packet->time_spent_in_Q1, bucket_token_count, (bucket_token_count > 1? "s":""));

	//isEmpty = My402ListEmpty(&Q2PacketList);
	// Adding the packet to queue Q2
	My402ListAppend(&queue_Q2, temp_packet);
	gettimeofday(&(temp_packet->arrival_time_Q2), NULL);
	packet_time = (format_time(temp_packet->arrival_time_Q2) - format_time(emulation_start_time))/1000;
	fprintf(stdout, "%012.3fms: p%d enters Q2\n",packet_time, temp_packet->packet_id);

	 if ( My402ListLength(&queue_Q2) >= 1 )
		 pthread_cond_broadcast(&execute_service);
}

void token_bucket_filter(params* token_parameters)
{
	double token_time;
	struct timeval current_time;

	for(;;)
	{
		usleep((token_parameters->token_inter_arrival_time)*1000);

		//Checking if all packets have been processed
		if( stop1 == 1 && My402ListEmpty(&queue_Q1))
		{
			stop2 = 1;
			pthread_cond_broadcast(&execute_service);
			break;
		}

		char* plural_character;
		// Create token
		token_serial_number += 1;
		gettimeofday(&current_time, NULL);
		token_time = (format_time(current_time) - format_time(emulation_start_time))/1000;

		pthread_mutex_lock(&token_bucket_lock);
		// Checking for overflow
		if (bucket_token_count < token_parameters->token_bucket_depth)
		{
			// Adding the token to the token bucket
			bucket_token_count +=1;
			if(bucket_token_count > 1)
				plural_character = "s";
			else
				plural_character = "";

			fprintf(stdout,"%012.3fms: token t%d arrives, token bucket now has %d token%s\n", token_time, token_serial_number, bucket_token_count, plural_character);

			My402ListElem* first_elem = My402ListFirst(&queue_Q1);

			if(first_elem != NULL)
			{
				if(((pack*)first_elem->obj)->tokens_required <= bucket_token_count)
				{
					move_from_Q1_to_Q2(first_elem);
				}
				pthread_mutex_unlock(&token_bucket_lock);
		    }
			else
			{
				pthread_mutex_unlock(&token_bucket_lock);
			}
		}
		else
		{
			tokens_dropped += 1;
			fprintf(stdout,"%012.3fms: token t%d arrives, dropped\n",token_time, token_serial_number);
			pthread_mutex_unlock(&token_bucket_lock);
		}


	}
	pthread_exit(NULL);
}

void set_trace_driven_parameters_for_packet(pack* temp_packet, params* temp_packet_parameters)
{
	// Setting the packet_id
	temp_packet->packet_id = packet_serial_number;

	char line[1024];
	char *token;
	if(fgets(line, sizeof(line), file_pointer) != NULL)
	{
		if(isspace(*line) || *line == '\t')
		{
			fprintf(stderr, "Error: Input file not in right format ! Record contains leading spaces\n");
			exit(0);
		}

		// Setting packet_inter_arrival_time
		token = strtok(line, " \t");
		if(token != NULL)
		{
			temp_packet->inter_arrival_time = atof(token);
			if(temp_packet->inter_arrival_time <= 0)
			{
				fprintf(stderr, "Error: Input file not in right format ! Invalid value for parameter inter-arrival time. Should be a positive real number.\n");
				exit(0);
			}
		}
		else
		{
			fprintf(stderr, "Error: Input file not in right format ! Error while parsing file parameters");
			exit(0);
		}

		// Setting tokens_required_by_packet
		token = strtok(NULL, " \t");
		if (token != NULL)
		{
			temp_packet->tokens_required = atoi(token);
			if(temp_packet->tokens_required <= 0)
			{
				fprintf(stderr, "Error: Input file not in right format ! Invalid value for parameter tokens required by packet. Should be a positive integer less than 2147483648.\n");
				exit(0);
			}
		}
		else
		{
			fprintf(stderr, "Error: Input file not in right format ! Error while parsing file parameters");
			exit(0);
		}

		// Setting packet_service_time
		token = strtok(NULL, " \t");
		if (token != NULL)
		{
			temp_packet->service_time = atof(token);
			if(temp_packet->service_time <= 0)
			{
				fprintf(stderr, "Error: Input file not in right format ! Invalid value for parameter service time requested by packet. Should be a positive real number.\n");
				exit(0);
			}
		}
		else
		{
			fprintf(stderr, "Error: Input file not in right format ! Error while parsing file parameters");
			exit(0);
		}

		// Checking for extra parameters
		token = strtok(NULL, " \t");
		if(token != NULL)
		{
			printf("Error: Input file not in right format ! Extra parameters in the line\n");
			exit(1);
		}
	}

	if(packet_serial_number == temp_packet_parameters->num_of_packets)
	{
		fclose(file_pointer);
	}

	// Allocating memory to other parameters
	memset(&(temp_packet->arrival_time),0,sizeof(struct timeval));
	memset(&(temp_packet->arrival_time_Q1),0,sizeof(struct timeval));
	memset(&(temp_packet->departure_time_Q1),0,sizeof(struct timeval));
	memset(&(temp_packet->arrival_time_Q2),0,sizeof(struct timeval));
	memset(&(temp_packet->departure_time_Q2),0,sizeof(struct timeval));
	memset(&(temp_packet->exit_time),0,sizeof(struct timeval));
	temp_packet->time_spent_in_Q1 = 0;
	temp_packet->time_spent_in_Q2 = 0;
	temp_packet->actual_inter_arrival_time = 0;
	temp_packet->actual_service_time = 0;
	temp_packet->time_in_system = 0;
}

void set_deterministic_parameters_for_packet(pack* temp_packet, params* temp_packet_parameters)
{
	//Setting the packet_id
	temp_packet->packet_id = packet_serial_number;

	//Setting packet_inter_arrival_time
	double packet_inter_arrival_time = ((double)1/temp_packet_parameters->packet_arrival_rate)*1000;
	if(packet_inter_arrival_time <= 10000)
		temp_packet->inter_arrival_time = packet_inter_arrival_time;
	else
		temp_packet->inter_arrival_time = 10000;

	//Setting packet_service_time
	double service_time = ((double)1/temp_packet_parameters->service_rate)*1000;
	if( service_time <= 10000)
		temp_packet->service_time = service_time;
	else
		temp_packet->service_time = 10000;

	//Setting tokens_required_by_packet
	temp_packet->tokens_required = temp_packet_parameters->tokens_required_by_packet;

	// Allocating memory to other parameters
	memset(&(temp_packet->arrival_time),0,sizeof(struct timeval));
	memset(&(temp_packet->arrival_time_Q1),0,sizeof(struct timeval));
	memset(&(temp_packet->departure_time_Q1),0,sizeof(struct timeval));
	memset(&(temp_packet->arrival_time_Q2),0,sizeof(struct timeval));
	memset(&(temp_packet->departure_time_Q2),0,sizeof(struct timeval));
	memset(&(temp_packet->exit_time),0,sizeof(struct timeval));
	temp_packet->time_spent_in_Q1 = 0;
	temp_packet->time_spent_in_Q2 = 0;
	temp_packet->actual_inter_arrival_time = 0;
	temp_packet->actual_service_time = 0;
	temp_packet->time_in_system = 0;
}

void queue_1(params* packet_parameters)
{
	int i;
	double packet_time;
	struct timeval current_time;

	previous_packet_arrival_time = emulation_start_time;

	for(i = 1; i <= packet_parameters->num_of_packets; i++)
	{
		pack* new_packet = (pack*)malloc(sizeof(pack));
		packet_serial_number +=1;

		// Create packet
		if(is_tracedriven == 1)
			set_trace_driven_parameters_for_packet(new_packet, packet_parameters);
		else
			set_deterministic_parameters_for_packet(new_packet, packet_parameters);

		usleep((new_packet->inter_arrival_time)*1000);

		gettimeofday(&(new_packet->arrival_time), NULL);

		new_packet->actual_inter_arrival_time = (format_time(new_packet->arrival_time) - format_time(previous_packet_arrival_time))/1000;
		previous_packet_arrival_time = new_packet->arrival_time;
		// Calculating sum for statistics
		packet_inter_arrival_time_sum += new_packet->actual_inter_arrival_time;

		packet_time = (format_time(new_packet->arrival_time) - format_time(emulation_start_time))/1000;

		pthread_mutex_lock(&token_bucket_lock);
		// Checking if token required is greater than bucket depth
		if (new_packet->tokens_required <= packet_parameters->token_bucket_depth)
		{
			fprintf(stdout,"%012.3fms: p%d arrives, needs %d token%s, inter-arrival time = %.3fms\n", packet_time, new_packet->packet_id, new_packet->tokens_required, (new_packet->tokens_required > 1 ? "s":"") ,new_packet->actual_inter_arrival_time);

			// Adding the token to the Queue Q1
			My402ListAppend(&queue_Q1, new_packet);
			gettimeofday(&(new_packet->arrival_time_Q1), NULL);
			packet_time = (format_time(new_packet->arrival_time_Q1) - format_time(emulation_start_time))/1000;
			fprintf(stdout, "%012.3fms: p%d enters Q1\n",packet_time, new_packet->packet_id);

			My402ListElem* first_elem = My402ListFirst(&queue_Q1);

			if(((pack*)first_elem->obj)->tokens_required <= bucket_token_count)
			{
				move_from_Q1_to_Q2(first_elem);
			}
			pthread_mutex_unlock(&token_bucket_lock);
		}
		else
		{
			gettimeofday(&current_time, NULL);
			new_packet->service_time = (format_time(current_time) - format_time(new_packet->arrival_time))/1000;
			free(new_packet);
			discarded_packets += 1;
			fprintf(stdout,"%012.3fms: p%d arrives, needs %d token%s, inter-arrival time = %.3fms, dropped\n", packet_time, new_packet->packet_id, new_packet->tokens_required, (new_packet->tokens_required > 1 ? "s":""), new_packet->inter_arrival_time);
			pthread_mutex_unlock(&token_bucket_lock);
		}

		if ( i == packet_parameters->num_of_packets )
		{
				stop1 = 1;
		}
	}
}

void service_S1(params* program_parameters)
{
	double time;

	while(1)
	{
		pthread_mutex_lock(&token_bucket_lock);
		while(My402ListLength(&queue_Q2) == 0 && ( stop1 == 0 || stop2 == 0 ))
		{
			pthread_cond_wait(&(execute_service), &(token_bucket_lock));
		}
		if ( My402ListLength(&queue_Q2) > 0 )
		{
			My402ListElem* first_elem = My402ListFirst(&queue_Q2);
			pack* temp_packet = (pack*)first_elem->obj;
			My402ListUnlink(&queue_Q2, first_elem);
			pthread_mutex_unlock(&token_bucket_lock);
			gettimeofday(&(temp_packet->departure_time_Q2), NULL);
			temp_packet->time_spent_in_Q2 = (format_time(temp_packet->departure_time_Q2) - format_time(temp_packet->arrival_time_Q2))/1000;

			time = (format_time(temp_packet->departure_time_Q2) - format_time(emulation_start_time))/1000;
			fprintf(stdout,"%012.3fms: p%d leaves Q2, time in Q2 = %.3fms\n",time, temp_packet->packet_id, temp_packet->time_spent_in_Q2);
			fprintf(stdout,"%012.3fms: p%d begins service at S1, requesting %.3fms of service\n",time, temp_packet->packet_id, temp_packet->service_time);

			usleep((temp_packet->service_time)*1000);

			gettimeofday(&(temp_packet->exit_time), NULL);
			temp_packet->time_in_system = (format_time(temp_packet->exit_time) - format_time(temp_packet->arrival_time))/1000;
			time = (format_time(temp_packet->exit_time) - format_time(emulation_start_time))/1000;
			temp_packet->actual_service_time = (format_time(temp_packet->exit_time) - format_time(temp_packet->departure_time_Q2))/1000;
			packets_completed += 1;
			fprintf(stdout,"%012.3fms: p%d departs from S1, service time = %.3fms, time in system = %.3fms\n", time, temp_packet->packet_id, temp_packet->actual_service_time, temp_packet->time_in_system);

			//Calculating the statistics values
			total_time_spent_in_Q1 += temp_packet->time_spent_in_Q1;
			total_time_spent_in_Q2 += temp_packet->time_spent_in_Q2;
			sum_packet_service_time_in_S1 += temp_packet->actual_service_time;
			sum_time_packet_spent_in_system += temp_packet->time_in_system;
			sum_of_time_in_system_square += temp_packet->time_in_system * temp_packet->time_in_system;
		}
		else if ( My402ListLength(&queue_Q2) == 0 && stop1 == 1 && stop2 == 1 )
		{
				pthread_mutex_unlock(&token_bucket_lock);
				stop3 = 1;
				break;
		}
	}
}

void service_S2(params* program_parameters)
{
	double time;

	while(1)
	{
		pthread_mutex_lock(&token_bucket_lock);
		while(My402ListLength(&queue_Q2) == 0 && ( stop1 == 0 || stop2 == 0 ))
		{
			pthread_cond_wait(&(execute_service), &(token_bucket_lock));
		}
		if ( My402ListLength(&queue_Q2) > 0 )
		{
			My402ListElem* first_elem = My402ListFirst(&queue_Q2);
			pack* temp_packet = (pack*)first_elem->obj;
			My402ListUnlink(&queue_Q2, first_elem);
			pthread_mutex_unlock(&token_bucket_lock);
			gettimeofday(&(temp_packet->departure_time_Q2), NULL);
			temp_packet->time_spent_in_Q2 = (format_time(temp_packet->departure_time_Q2) - format_time(temp_packet->arrival_time_Q2))/1000;

			time = (format_time(temp_packet->departure_time_Q2) - format_time(emulation_start_time))/1000;
			fprintf(stdout,"%012.3fms: p%d leaves Q2, time in Q2 = %.3fms\n",time, temp_packet->packet_id, temp_packet->time_spent_in_Q2);
			fprintf(stdout,"%012.3fms: p%d begins service at S2, requesting %.3fms of service\n",time, temp_packet->packet_id, temp_packet->service_time);

			usleep((temp_packet->service_time)*1000);

			gettimeofday(&(temp_packet->exit_time), NULL);
			temp_packet->time_in_system = (format_time(temp_packet->exit_time) - format_time(temp_packet->arrival_time))/1000;
			time = (format_time(temp_packet->exit_time) - format_time(emulation_start_time))/1000;
			temp_packet->actual_service_time = (format_time(temp_packet->exit_time) - format_time(temp_packet->departure_time_Q2))/1000;
			packets_completed += 1;
			fprintf(stdout,"%012.3fms: p%d departs from S2, service time = %.3fms, time in system = %.3fms\n", time, temp_packet->packet_id, temp_packet->actual_service_time, temp_packet->time_in_system);

			//Calculating the statistics values
			total_time_spent_in_Q1 += temp_packet->time_spent_in_Q1;
			total_time_spent_in_Q2 += temp_packet->time_spent_in_Q2;
			sum_packet_service_time_in_S2 += temp_packet->actual_service_time;
			sum_time_packet_spent_in_system += temp_packet->time_in_system;
			sum_of_time_in_system_square += temp_packet->time_in_system * temp_packet->time_in_system;
		}
		else if ( My402ListLength(&queue_Q2) == 0 && stop1 == 1 && stop2 == 1 )
		{
				pthread_mutex_unlock(&token_bucket_lock);
				stop4 = 1;
				break;
		}
	}
}

void emulation_statistics(params* packet_parameters)
{
	double packet_service_time_sum = sum_packet_service_time_in_S1 + sum_packet_service_time_in_S2;

	double average_packet_inter_arrival_time = (packet_inter_arrival_time_sum)/(packet_serial_number);
	double average_packet_service_time = (packet_service_time_sum)/(packets_completed);

	double total_emulation_time = (format_time(emulation_end_time) - format_time(emulation_start_time))/1000;

	double average_no_of_packets_in_Q1 = (total_time_spent_in_Q1)/(total_emulation_time);
	double average_no_of_packets_in_Q2 = (total_time_spent_in_Q2)/(total_emulation_time);
	double average_no_of_packets_in_S1 = (sum_packet_service_time_in_S1)/(total_emulation_time);
	double average_no_of_packets_in_S2 = (sum_packet_service_time_in_S2)/(total_emulation_time);

	double average_time_packet_spent_in_system = (sum_time_packet_spent_in_system)/packets_completed;

	double average_of_time_in_system_square = (sum_of_time_in_system_square)/(packets_completed);
	double std_dev_for_time_spent_in_system = sqrt(average_of_time_in_system_square - (average_time_packet_spent_in_system * average_time_packet_spent_in_system));

	double token_drop_probability = ((double)tokens_dropped)/token_serial_number;
	double packet_drop_probability = ((double)discarded_packets)/packet_serial_number;

	fprintf(stdout,"Statistics:\n\n");
	if(packet_serial_number!= 0)
	{
		fprintf(stdout,"\taverage packet inter-arrival time = %.6g sec\n",(average_packet_inter_arrival_time/1000));
	}
	else
	{
		fprintf(stdout,"\taverage packet inter-arrival time = 0 : No packets arrived\n");
	}

	if(packets_completed != 0)
	{
		fprintf(stdout,"\taverage packet service time = %.6g sec\n\n",(average_packet_service_time/1000));
	}
	else
	{
		fprintf(stdout,"\taverage packet service time = 0 : No packets serviced successfully\n\n");
	}

	if(total_emulation_time != 0)
	{
		fprintf(stdout,"\taverage number of packets in Q1 = %.6g packets\n",average_no_of_packets_in_Q1);
		fprintf(stdout,"\taverage number of packets in Q2 = %.6g packets\n",average_no_of_packets_in_Q2);
		fprintf(stdout,"\taverage number of packets at S1 = %.6g packets\n",average_no_of_packets_in_S1);
		fprintf(stdout,"\taverage number of packets at S2 = %.6g packets\n\n",average_no_of_packets_in_S2);
	}
	else
	{
		fprintf(stdout,"\taverage number of packets in Q1 = 0 : Emulation Time is zero\n");
		fprintf(stdout,"\taverage number of packets in Q2 = 0 : Emulation Time is zero\n");
		fprintf(stdout,"\taverage number of packets at S1 = 0 : Emulation Time is zero\n");
		fprintf(stdout,"\taverage number of packets at S2 = 0 : Emulation Time is zero\n\n");
	}

	if(packets_completed != 0)
	{
		fprintf(stdout,"\taverage time a packet spent in system = %.6g sec\n",(average_time_packet_spent_in_system/1000));
		fprintf(stdout,"\tstandard deviation for time spent in system = %.6g sec\n\n",(std_dev_for_time_spent_in_system/1000));
	}
	else
	{
		fprintf(stdout,"\taverage time a packet spent in system = 0 : No packets serviced successfully\n");
		fprintf(stdout,"\tstandard deviation for time spent in system = 0 : No packets serviced successfully\n\n");
	}

	if(token_serial_number != 0)
	{
		fprintf(stdout,"\ttoken drop probability = %.6g\n",token_drop_probability);
	}
	else
	{
		fprintf(stdout,"\ttoken drop probability = N/A : No token arrived\n");
	}

	if(packet_serial_number != 0)
	{
		fprintf(stdout,"\tpacket drop probability = %.6g\n",packet_drop_probability);
	}
	else
	{
		fprintf(stdout,"\tpacket drop probability = N/A : No packets arrived\n");
	}
}

void interrupt_handler()
{
	struct timeval time_Q1, time_Q2;
	double current_time_Q1, current_time_Q2;

	My402ListElem* temp_Q1_elem = NULL;
	My402ListElem* temp_Q2_elem = NULL;

	pack* temp_Q1_packet = NULL;
	pack* temp_Q2_packet = NULL;

	sigwait(&set);

	pthread_mutex_lock(&token_bucket_lock);

	pthread_cancel(queue_Q1_thread);
	pthread_cancel(token_bucket_thread);

	fprintf(stdout, "\n\n----- Interrupt generated ! Clearing pending packets and terminating program. -----\n");

	while (!My402ListEmpty(&queue_Q1))
	{
		temp_Q1_elem = My402ListFirst(&queue_Q1);
		temp_Q1_packet = (pack*)temp_Q1_elem->obj;
		My402ListUnlink(&queue_Q1, temp_Q1_elem);

		gettimeofday(&time_Q1, NULL);
		current_time_Q1 = (format_time(time_Q1) - format_time(emulation_start_time))/1000;
		fprintf(stdout, "%012.3fms: p%d is removed from Q1.\n", current_time_Q1, temp_Q1_packet->packet_id);
	}

	while (!My402ListEmpty(&queue_Q2))
	{
		temp_Q2_elem = My402ListFirst(&queue_Q2);
		temp_Q2_packet = (pack*)temp_Q2_elem->obj;
		My402ListUnlink(&queue_Q2, temp_Q2_elem);

		gettimeofday(&time_Q2, NULL);
		current_time_Q2 = (format_time(time_Q2) - format_time(emulation_start_time))/1000;
		fprintf(stdout, "%012.3fms: p%d is removed from Q2.\n", current_time_Q2, temp_Q2_packet->packet_id);
	}
	pthread_cond_broadcast(&execute_service);

	pthread_mutex_unlock(&token_bucket_lock);
	stop1 = 1;
	stop2 = 1;
}

void read_command_line_arguments(int argc, char** argv,params* object)
{
	int counter = argc - 1;
	int parameters = 0, index = 0;
	memset(object,'\0',sizeof(params));
	parameters = getopt_long_only(argc, argv, ":",long_options, &index);

	while (parameters != -1)
	{
		switch(parameters)
		{
			case 'l': object->packet_arrival_rate = atof(optarg);
					  if(object->packet_arrival_rate == 0)
					  {
						  fprintf(stderr,"Error: Malformed command line ! Packet Arrival Rate should be a positive real number.\n"
								  "Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
						  exit(1);
					  }
					  break;

			case 'm': object->service_rate = atof(optarg);
					  if(object->service_rate == 0)
					  {
						  fprintf(stderr,"Error: Malformed command line ! Service Rate should be a positive real number.\n"
								  "Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
						  exit(1);
					  }
					  break;

			case 'r': object->token_arrival_rate = atof(optarg);
			 	 	  if(object->token_arrival_rate == 0)
					  {
						  fprintf(stderr,"Error: Malformed command line ! Token Arrival Rate should be a positive real number.\n"
								  "Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
						  exit(1);
					  }

					  double val = ((double)1/object->token_arrival_rate)*1000;
					  if(val > 10000)
						  object->token_inter_arrival_time = 10000;
					  else
						  object->token_inter_arrival_time = val;
					  break;

			case 'B': object->token_bucket_depth = atoi(optarg);

					  if((object->token_bucket_depth <= 0) || (object->token_bucket_depth > MAX_VAL))
					  {
						  fprintf(stderr,"Error: Malformed command line ! Token Bucket Depth should be a positive integer less than 2147483648.\n"
								  "Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				          exit(1);
					  }
					  break;

			case 'P': object->tokens_required_by_packet = atoi(optarg);

					  if((object->tokens_required_by_packet <= 0) || (object->tokens_required_by_packet > MAX_VAL))
					  {
						  fprintf(stderr,"Error: Malformed command line ! Tokens required by a packet should be a positive integer less than 2147483648.\n"
								  "Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				          exit(1);
					  }
					  break;

			case 'n': object->num_of_packets = atoi(optarg);

					  if((object->num_of_packets <= 0) || (object->num_of_packets > MAX_VAL))
					  {
					 	  fprintf(stderr,"Error: Malformed command line ! Number of Packets should be a positive integer less than 2147483648.\n"
								  "Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				          exit(1);
					  }
					  break;

			case 't': is_tracedriven = 1;
					  object->filename = optarg;
				      break;
			case ':':
			case '?':
			default : fprintf(stderr, "Error: Malformed command line !\n"
								"Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					  exit(-1);
					  break;
		}
		counter -= 2;
		parameters = getopt_long_only(argc, argv, ":", long_options, &index);
	}

	//Checking for any other random parameters in command line
	if(counter > 0)
	{
		fprintf(stderr, "Error: Malformed command line !\n"
					"Correct Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
		exit(1);
	}

	if(!object->packet_arrival_rate)
		object->packet_arrival_rate = 1;
	if(!object->service_rate)
		object->service_rate = 0.35;
	if(!object->token_arrival_rate)
	{
		object->token_arrival_rate = 1.5;
		object->token_inter_arrival_time = ((double)1/object->token_arrival_rate)*1000;
	}
	if(!object->token_bucket_depth)
		object->token_bucket_depth = 10;
	if(!object->tokens_required_by_packet)
		object->tokens_required_by_packet = 3;
	if(!object->num_of_packets)
		object->num_of_packets = 20;

	//Printing the parameters
	if(is_tracedriven == 1)
	{
		// Checking for the specified file
		struct stat s;
		if((access(object->filename, F_OK) ==-1))
		{
			fprintf(stderr,"Error: File %s does not exist.\n", object->filename);
			exit(1);
		}
		if((access(object->filename, R_OK) ==-1))
		{
			fprintf(stderr,"Error: Access Denied ! File %s does not have Read Permission.\n", object->filename);
			exit(1);
		}
		if(!stat(object->filename,&s))
		{
			if(s.st_mode & S_IFDIR)
			{
				fprintf(stderr,"Error: Given filename %s is a Directory.\n", object->filename);
				exit(1);
			}
		}
		file_pointer = fopen(object->filename,"r");
		if(file_pointer == NULL)
		{
			fprintf(stderr,"Error: File pointer is NULL. Exit !!\n");
			exit(1);
		}
		char line[1024];
		char *token;
		fgets(line, sizeof(line), file_pointer);
		token = strtok(line, "\t");
		if(token != NULL)
		{
			object->num_of_packets = atoi(token);
			if(object->num_of_packets <= 0)
			{
				fprintf(stderr, "Error: Input file not in right format ! Invalid value for parameter number of packets. Should be a positive integer value less than 2147483648.\n");
				exit(0);
			}
		}
		else
		{
			fprintf(stderr, "Error: Input file not in right format ! Error while parsing file parameters");
			exit(0);
		}

		fprintf(stdout,"Emulation Parameters:\n");

		fprintf(stdout,"\tnumber to arrive = %d\n\tr = %.6g\n\tB = %d\n\ttsfile = %s\n", object->num_of_packets, object->token_arrival_rate, object->token_bucket_depth,object->filename);
	}
	else
	{
		fprintf(stdout,"\tnumber to arrive = %d\n\tlambda = %.6g\n\tmu = %.6g\n\tr = %.6g\n\tB = %d\n\tP = %d\n", object->num_of_packets, object->packet_arrival_rate, object->service_rate, object->token_arrival_rate, object->token_bucket_depth, object->tokens_required_by_packet);
	}
}

int main (int argc, char** argv)
{
	double start_time = 0;
	double end_time = 0;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	params parameters;

	My402ListInit(&(queue_Q1));
	My402ListInit(&(queue_Q2));

	read_command_line_arguments(argc,argv,&parameters);

	gettimeofday(&emulation_start_time,0);
	fprintf(stdout, "%012.3fms: emulation begins\n",start_time);


	/* create a third thread which executes queue_Q1() */
	if(pthread_create(&queue_Q1_thread, NULL, (void*)queue_1, &parameters))
	{
		fprintf(stderr, "Error creating queue Q1 thread\n");
		return 1;
	}
	/* create a second thread which executes token_bucket_filter() */
	if(pthread_create(&token_bucket_thread, NULL, (void*)token_bucket_filter, &parameters))
	{
		fprintf(stderr, "Error creating token bucket thread\n");
		return 1;
	}
	/* create a fourth thread which executes service_S1() */
	if(pthread_create(&service_1, NULL, (void*)service_S1, &parameters))
	{
		fprintf(stderr, "Error creating service S1 thread\n");
		return 1;
	}
	/* create a fourth thread which executes service_S2() */
	if(pthread_create(&service_2, NULL, (void*)service_S2, &parameters))
	{
		fprintf(stderr, "Error creating service S2 thread\n");
		return 1;
	}
	/* create a fifth thread which executes the handler() */
	if(pthread_create(&handler_thread, NULL, (void*)interrupt_handler, NULL))
	{
		fprintf(stderr, "Error creating interrupt handler thread\n");
		return 1;
	}

	pthread_join(token_bucket_thread,0);
	pthread_join(queue_Q1_thread,0);
	pthread_join(service_1,0);
	pthread_join(service_2,0);

	// If all threads have finished execution then cancel interrupt thread
	if(stop1 == 1 && stop2 == 1 && stop3 == 1 && stop4 == 1)
		pthread_cancel(handler_thread);

	pthread_join(handler_thread, 0);

	gettimeofday(&emulation_end_time,0);
	end_time = (format_time(emulation_end_time) - format_time(emulation_start_time))/1000;
	fprintf(stdout, "%012.3fms: emulation ends\n\n", end_time);

	emulation_statistics(&parameters);
	My402ListUnlinkAll(&queue_Q1);
	My402ListUnlinkAll(&queue_Q2);
	return 0;
}
