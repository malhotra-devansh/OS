#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cs402.h"

#include "my402list.h"

#include <time.h>
#include <locale.h>




typedef struct transact
{
	char tran_type;
	time_t tran_time;
	long tran_amount;
	char* tran_description;
}transaction;

/*void PrintList(My402List* List)
{
	My402ListElem* temp = My402ListFirst(List);
	int length = My402ListLength(List);
	while(length)
	{
			printf("%c %d %d %s", ((transaction*)temp->obj)->tran_type,  (int)((transaction*)temp->obj)->tran_time,  ((transaction*)temp->obj)->tran_amount,  ((transaction*)temp->obj)->tran_description);
			temp = temp->next;
			length--;
	}
}*/


void GetFormattedValue(char* amount, char* final_amount)
{
	int i;
	int length = strlen(amount)+1;
	strcpy(final_amount,"(");
	for(i=1; i<= 13-length; i++)
	{
		strcat(final_amount," ");
	}
	strcat(final_amount,amount);
	strcat(final_amount,")");
}

void GetValueWithComma(char* value, char* amount_first_with_comma)
{
	int a, b;
	int j, k, l=1;
	char* string_reversed = (char*)malloc(sizeof(strlen(value)));
	char* string_correct = (char*)malloc(sizeof(strlen(value)));

	for(j = strlen(value)-1 , k = 0 ; j>=0 ; j--, k++,l++)
	{
		string_reversed[k]= value[j];
		if(l%3 ==0 && j>0)
			{
				k++;
				string_reversed[k]=',';
			}
	}
	string_reversed[k] = '\0';

	for(a = strlen(string_reversed)-1 , b = 0 ; a>=0 ; a--, b++)
	{
		string_correct[b] = string_reversed[a];
	}
	string_correct[b] = '\0';
	strcpy(amount_first_with_comma , string_correct);
}

void GetCorrectDate(char* tran_time, char* date )
{
	int i=0;
	char* pch;
	pch = strtok (tran_time," \n");
	while (pch != NULL)
	{
		if(i == 0)
		{
			strcpy(date, pch);
			strcat(date, " ");
		}
		if ((i == 1)|| (i == 2) || (i == 4))
		{
			strcat(date, pch);
			strcat(date, " ");
		}
		pch = strtok (NULL, " \n");
		i++;
	}
}

char* GetDescription(char* tran_description)
{
	char* pch;
	pch = strtok (tran_description,"\n");
	if(strlen(pch) > 24)
	{
	pch[24]='\0';
	}
	return pch;
}

char* GetAmount(long tran_amount, char tran_type)
{
	long transaction_amt;
	char* amount_first = (char*)malloc(sizeof(10));
	char* amount_last = (char*)malloc(sizeof(10));
	char* amount_last_one = (char*)malloc(sizeof(10));
	char* amount_last_two = (char*)malloc(sizeof(10));
	char* amount = (char*)malloc(sizeof(80));
	char* amount_first_with_comma = (char*)malloc(sizeof(80));
	char* final_amount = (char*)malloc(sizeof(14));
	sprintf(amount_last_one , "%ld", tran_amount%10);
	transaction_amt = tran_amount/10;
	sprintf(amount_last_two , "%ld", transaction_amt%10);
	strcpy(amount_last,amount_last_two);
	strcat(amount_last,amount_last_one);

	sprintf(amount_first , "%ld", tran_amount/100);

	GetValueWithComma(amount_first, amount_first_with_comma);
	strcpy(amount, amount_first_with_comma);
	strcat(amount,".");
	strcat(amount, amount_last);

	if(tran_type == '-')
	{
		GetFormattedValue(amount, final_amount);
		return final_amount;
	}
	strcat(amount, " ");
	return amount;
}

char* GetBalance(long current_balance, char tran_type)
{
	long cur_bal;
	long current_bal= current_balance;
	char* balance_first = (char*)malloc(sizeof(char)*10);
	char* balance_last = (char*)malloc(sizeof(char)*10);
	char* balance_last_one = (char*)malloc(sizeof(char)*10);
	char* balance_last_two = (char*)malloc(sizeof(char)*10);
	char* balance = (char*)malloc(sizeof(char)*20);
	char* balance_first_with_comma = (char*)malloc(sizeof(char)*20);
	char* final_amount = (char*)malloc(sizeof(14));
	sprintf(balance_last , "%d", (abs(current_bal))%100);

	sprintf(balance_last_one , "%d", (abs(current_bal))%10);
	cur_bal = (abs(current_bal))/10;
	sprintf(balance_last_two , "%ld", cur_bal%10);

	strcpy(balance_last,balance_last_two);
	strcat(balance_last,balance_last_one);

	sprintf(balance_first , "%d", (abs(current_bal))/100);

	/*if(strlen(balance_last) == 1)
	{
		strcat(balance_last, "0");
	}*/
	GetValueWithComma(balance_first, balance_first_with_comma);
	strcpy(balance, balance_first_with_comma);
	strcat(balance,".");
	strcat(balance, balance_last);
	if(current_balance < 0)
	{
		GetFormattedValue(balance, final_amount);
		return final_amount;
	}
	strcat(balance, " ");
	return balance;
}

char* GetBalanceString(char tran_type, long amount)
{
	static long current_balance = 0;
	char* final_amount = (char*)malloc(sizeof(14));
	if(tran_type == '-')
	{
		current_balance = current_balance - amount;
		final_amount = GetBalance(current_balance, tran_type);
	}
	else
	{
		current_balance = current_balance + amount;
		final_amount = GetBalance(current_balance, tran_type);
	}
	return final_amount;
}

void Display(My402List* List)
{
	printf("+-----------------+--------------------------+----------------+----------------+\n");
    printf("|       Date      | Description              |         Amount |        Balance |\n");
    printf("+-----------------+--------------------------+----------------+----------------+\n");

    My402ListElem* temp = My402ListFirst(List);
    int length = My402ListLength(List);
    //static long current_balance = 0;
    while(length)
    {

    	char* description;
    	char* amount;
    	char* balance;

    	/* Putting date in correct format*/
    	char date[30];
    	struct tm *loctime;
    	loctime = localtime (&((transaction*)temp->obj)->tran_time);
    	strftime (date, 30, "%a %b %e %Y", loctime);

    	/* Taking description*/
    	description = GetDescription(((transaction*)temp->obj)->tran_description);

    	/* Getting the formatted amount value */
    	amount = GetAmount(((transaction*)temp->obj)->tran_amount , ((transaction*)temp->obj)->tran_type);

    	/* Getting the correct and formatted value for Balance */
    	balance = GetBalanceString(((transaction*)temp->obj)->tran_type, ((transaction*)temp->obj)->tran_amount);

   		printf("| %15s | %-24s | %14s | %14s |\n", date, description, amount, balance);
   		temp = temp->next;
		length--;
   	}
    printf("+-----------------+--------------------------+----------------+----------------+\n");
}

void BubbleForward(My402List *pList, My402ListElem **pp_elem1, My402ListElem **pp_elem2)
    /* (*pp_elem1) must be closer to First() than (*pp_elem2) */
{
    My402ListElem *elem1=(*pp_elem1), *elem2=(*pp_elem2);
    void *obj1=elem1->obj, *obj2=elem2->obj;
    My402ListElem *elem1prev=My402ListPrev(pList, elem1);
/*  My402ListElem *elem1next=My402ListNext(pList, elem1); */
/*  My402ListElem *elem2prev=My402ListPrev(pList, elem2); */
    My402ListElem *elem2next=My402ListNext(pList, elem2);

    My402ListUnlink(pList, elem1);
    My402ListUnlink(pList, elem2);
    if (elem1prev == NULL) {
        (void)My402ListPrepend(pList, obj2);
        *pp_elem1 = My402ListFirst(pList);
    } else {
        (void)My402ListInsertAfter(pList, obj2, elem1prev);
        *pp_elem1 = My402ListNext(pList, elem1prev);
    }
    if (elem2next == NULL) {
        (void)My402ListAppend(pList, obj1);
        *pp_elem2 = My402ListLast(pList);
    } else {
        (void)My402ListInsertBefore(pList, obj1, elem2next);
        *pp_elem2 = My402ListPrev(pList, elem2next);
    }
}

void BubbleSortForwardList(My402List *pList, int num_items)
{
    My402ListElem *elem=NULL;
    int i=0;

    if (My402ListLength(pList) != num_items) {
        fprintf(stderr, "List length is not %1d in BubbleSortForwardList().\n", num_items);
        exit(1);
    }
    for (i=0; i < num_items; i++) {
        int j=0, something_swapped=FALSE;
        My402ListElem *next_elem=NULL;

        for (elem=My402ListFirst(pList), j=0; j < num_items-i-1; elem=next_elem, j++) {
            int cur_val=(int)((transaction*)elem->obj)->tran_time, next_val=0;

            next_elem=My402ListNext(pList, elem);
            next_val = (int)((transaction*)next_elem->obj)->tran_time;

            if (cur_val == next_val)
                       {
                       	perror("Error: Illegal input. Identical timestamps !!\n");
                       		    	exit(1);
                       }
            if (cur_val > next_val) {
                BubbleForward(pList, &elem, &next_elem);
                something_swapped = TRUE;
            }
        }
        if (!something_swapped) break;
    }
}

void SeperateIntegerAndDecimalValues(char *amount_value, long *current_amount)
{
	char *integer_part=(char*)malloc(sizeof(char) * (20));
		char *decimal_part=(char*)malloc(sizeof(char) * (10));
		integer_part=strtok(amount_value,".");
		decimal_part=strtok(NULL,"\n");
		if(strlen(decimal_part) > 2)
		{
		    	perror("Error: Malformed input , Illegal amount value !!\n");
		    	exit(1);
			}

		strcat(integer_part,decimal_part);
		*current_amount=(long)atol(integer_part);
		return;
}

int main (int argc, char** argv)
{
	  //char filename[100];
	  FILE *file = NULL;

	  /*if(argc == 0)
	  {
		  file = stdin;
	  }
	  if(argc == 1)
	  {
		  perror("Error: Filename absent !!\n");
	  }
	  if(argc > 2)
	  {
		  perror("Error: Extra parameters !!\n");
	  }*/

	  My402List List;
	  My402ListInit(&(List));
	  //char* filename;
	  //char buffer[256];
	  //printf ("Enter filename: ");
	  //fgets (buffer, 256, stdin);
	  //buffer[strcspn(buffer, "\n")] = 0;

	 /* if(buffer == NULL)
	  {
		  filename = "test.tfile";
	  }
	  else
	  {
		  filename = buffer;
	  }*/

	  //char filename[] = "test.tfile";


	if(argc==2)
	  {
		  file=stdin;
	  }
	  else if(argc == 3)
	  {
		  file = fopen (argv[2], "r" );
	  }



	  if (file != NULL)
	  {
	    char line [1024];
	    /*Reading a line*/
	    while(fgets(line,sizeof line,file)!= NULL)
	    {
	    	int character_count = 0;
	    	int i=0;
	    	char * pch;
	    	char *ptr[8];
	    					//printf ("Splitting string \"%s\" into tokens:\n",line);
	    					pch = strtok (line,"\t\n");
	    					while (pch != NULL)
	    					{
	    						if(i>=4)
	    						{
	    							perror("Error: Invalid input. Too many arguments !!\n");
	    															exit(1);
	    						}
	    					  ptr[i]=pch;
	    					  if(strlen(ptr[i])== 0)
	    					  {

	    					  }
	    					  //printf ("%s\n",pch);
	    					  pch = strtok (NULL, "\t\n");
	    					  character_count = character_count + strlen(ptr[i]);
	    					  i++;
	    					}
	    					if(i<3)
	    					{
	    						perror("Error: Invalid input. Arguments missing !!\n");
	    														exit(1);
	    					}
	        // Checking for valid line length
	        if(character_count > 80)
	        {
	        	perror("Error: More than 80 characters. Invalid line !!\n");
	        	exit(1);
	        }

	        transaction *trans = (transaction*)malloc(sizeof(transaction));

	        // Checking whether transaction is of valid type
	        if(*ptr[0] == '+' || *ptr[0] == '-')
	        {
	        trans->tran_type = *ptr[0];
	        }
	        else
	        {
	        	perror("Error: Malformed input , Illegal transaction type !!\n");
	        	exit(1);
	        }

	        // Checking if timestamp value specified is valid
	        if(strlen(ptr[1]) <= 10)
	        {
	        	int timestmp;
	        					timestmp = atoi(ptr[1]);
	        						if ( timestmp >= 0 && timestmp <= time(NULL))
	        						{
	        							trans->tran_time = atoi(ptr[1]);
	        						}
	        						else
	        						{
	        							perror("Error: Malformed input , Illegal timestamp !!\n");
	        											exit(1);
	        						}
	        }
	        else
	        {
	        	perror("Error: Malformed input , Illegal timestamp !!\n");
	        	exit(1);
	        }

	        // Checking for number of digits before and after decimal point in the given amount
	        /*char* pch_digits;
	        char *ptr_digits[2];
	        char* str;
	        strcpy(str, ptr[2]);

			int j = 0;
	        pch_digits = strtok (str,".\n");
	        while (pch_digits != NULL)
	        {
	        	ptr_digits[j] = pch_digits;
				pch_digits = strtok (NULL, ".\n");
				j++;
	        }
	        if(strlen(ptr_digits[1]) <= 2 && strlen(ptr_digits[0]) >= 1 && strlen(ptr_digits[0]) <= 7)
	        {
	        	trans->tran_amount = (atof(ptr[2]))*100;

	        	if(trans->tran_amount < 0)
	        	{
	        		perror("Error: Malformed input , Amount value negative !!\n");
	        		exit(1);
	        	}
	        }

	        else
	        {
	        	perror("Error: Malformed input , Illegal amount value !!\n");
	        	exit(1);
	        }*/

	        //trans->tran_amount = (atof(ptr[2]))*100;
	        char *amount_value=(char*)malloc(sizeof(char) * 14);
	        long current_amount;
	        amount_value=strtok(ptr[2],"\t");
	        SeperateIntegerAndDecimalValues(amount_value,&current_amount);
	        if(current_amount > 0)
	        {
	        	 trans->tran_amount = current_amount;
	        }
	        else
	        {
	        	perror("Error: Malformed input , Negative amount !!\n");
	        	exit(1);
	        }

	        // Checking if description is NULL
	        trans->tran_description = (char*) malloc(sizeof(char)* 25);
	        //trans->tran_description = ptr[3];
	        trans->tran_description = strdup(ptr[3]);

	        //printf("%c %d %d %s", trans->tran_type,  (int)trans->tran_time,  trans->tran_amount,  trans->tran_description);
	        My402ListAppend(&(List), trans);
	    }

	    fclose(file);

	  }

	  else
	  {
	    perror("Error: File pointer null !!\n"); //print the error message on stderr.
	  }
	  //PrintList(&(List));
	  int items = My402ListLength (&(List));
	  BubbleSortForwardList(&(List), items);
	  //PrintList(&(List));
	  Display(&(List));
	  exit(1);
}
