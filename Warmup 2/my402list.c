#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include "cs402.h"
#include "my402list.h"

/*Returns the number of elements in the list.*/
int  My402ListLength(My402List* List)
{
	return List->num_members;
}

/*Returns TRUE if the list is empty. Returns FALSE otherwise.*/
int  My402ListEmpty(My402List* List)
{
	if( List->num_members == 0)
		return TRUE;
	else
		return FALSE;
}

/*If list is empty, just add obj to the list. Otherwise, add obj after Last().
 *This function returns TRUE if the operation is performed successfully and returns FALSE otherwise.*/
int  My402ListAppend(My402List* List, void* obj)
{
	 My402ListElem* temp= (My402ListElem*) malloc(sizeof(My402ListElem));
	 if(temp==NULL)
	 		return FALSE;
	 temp->obj = obj;
	 // When list is empty
	if( List->num_members == 0)
	{
		List->anchor.next=temp;
		List->anchor.prev=temp;
		temp->prev = &(List->anchor);
		temp->next = &(List->anchor);
		List->num_members += 1;
		return TRUE;
	}
	// When list has members
	else
	{
		My402ListElem* last= My402ListLast(List);
		last->next = temp;
		temp->prev = last;
		temp->next = &(List->anchor);
		List->anchor.prev=temp;
		List->num_members += 1;
		return TRUE;
	}
		return FALSE;
}

/*If list is empty, just add obj to the list. Otherwise, add obj before First().
 *This function returns TRUE if the operation is performed successfully and returns FALSE otherwise.*/
int  My402ListPrepend(My402List* List, void* obj)
{
	My402ListElem* temp= (My402ListElem*) malloc(sizeof(My402ListElem));
	if(temp==NULL)
		return FALSE;
	temp->obj = obj;
	// When list is empty
	if( List->num_members == 0)
	{
		List->anchor.next=temp;
		List->anchor.prev=temp;
		temp->prev = &(List->anchor);
		temp->next = &(List->anchor);
		List->num_members += 1;
		return TRUE;
	}
	// When list has members
	else
	{
		My402ListElem* first= My402ListFirst(List);
		first->prev = temp;
		temp->next = first;
		temp->prev = &(List->anchor);
		List->anchor.next=temp;
		List->num_members += 1;
		return TRUE;
	}
	return FALSE;
}

/*Unlinks and deletes elem from the list.*/
void My402ListUnlink(My402List* List, My402ListElem* Elem)
{
		Elem->next->prev= Elem->prev;
		Elem->prev->next= Elem->next;
		free(Elem);
		List->num_members -=1;
		return;
}

/*Unlinks and deletes all elements from the list and makes the list empty.*/
void My402ListUnlinkAll(My402List* List)
{
	My402ListElem* temp1;
	My402ListElem* temp2;

	if(List == NULL)
		return;
	else if( My402ListFirst(List) == NULL)
		return;
	else
	{
		while(List->num_members > 1)
		{
			temp1= List->anchor.next;
			List->anchor.next= temp1->next;
			temp1->next->prev= &(List->anchor);
			free(temp1);
			List->num_members -=1;
		}
		temp2= List->anchor.next;
		List->anchor.next= &(List->anchor);
		List->anchor.prev= &(List->anchor);
		free(temp2);
		List->num_members = 0;
		return;
	}
}

/*Inserts obj between elem and elem->next */
int  My402ListInsertAfter(My402List* List, void* obj, My402ListElem* Elem)
{
	My402ListElem *temp=(My402ListElem*) malloc(sizeof(My402ListElem));
	if(temp==NULL)
		return FALSE;

	if(Elem == NULL)
	{
		return(My402ListAppend(List,obj));
	}
	temp->obj= obj;
	temp->next= Elem->next;
	temp->prev= Elem;
	Elem->next->prev= temp;
	Elem->next= temp;
	List->num_members +=1;
	return TRUE;
}


/*Inserts obj between elem and elem->prev */
int  My402ListInsertBefore(My402List* List, void* obj, My402ListElem* Elem)
{
	My402ListElem *temp=(My402ListElem*) malloc(sizeof(My402ListElem));
	if(temp==NULL)
			return FALSE;

	if(Elem == NULL)
	{
		return(My402ListPrepend(List,obj));
	}

	temp->obj= obj;
	temp->next= Elem;
	temp->prev= Elem->prev;
	Elem->prev->next= temp;
	Elem->prev= temp;
	List->num_members +=1;
	return TRUE;
}

/*Returns the first list element or NULL if the list is empty.*/
My402ListElem *My402ListFirst(My402List* List)
{
	if( List->num_members == 0)
		return NULL;
	else
		return List->anchor.next;
}

/*Returns the last list element or NULL if the list is empty.*/
My402ListElem *My402ListLast(My402List* List)
{
	if( List->num_members == 0)
			return NULL;
		else
			return List->anchor.prev;
}

/*Returns elem->next or NULL if elem is the last item on the list.*/
My402ListElem *My402ListNext(My402List* List, My402ListElem* Elem)
{
	My402ListElem* last= My402ListLast(List);
	if(Elem->obj== last->obj)
		return NULL;
	else
		return Elem->next;
}

/*Returns elem->prev or NULL if elem is the first item on the list.*/
My402ListElem *My402ListPrev(My402List* List, My402ListElem* Elem)
{
	My402ListElem* first= My402ListFirst(List);
		if(Elem->obj== first->obj)
			return NULL;
		else
			return Elem->prev;
}

/*Returns the list element elem such that elem->obj == obj.*/
My402ListElem *My402ListFind(My402List* List, void* obj)
{
	My402ListElem* temp= My402ListFirst(List);
	int length= My402ListLength(List);
	while(length)
	{
		if(temp->obj == obj)
			return temp;
		else
		{
			temp= temp->next;
			length--;
		}
	}
	return NULL;
}

/*Initialize the list into an empty list. Returns TRUE if all is well and returns FALSE if there is an error initializing the list.*/
int My402ListInit(My402List* List)
{
	if(List == NULL)
		return FALSE;
	else
	{
		List->num_members= 0;
		List->anchor.next= &(List->anchor);
		List->anchor.prev= &(List->anchor);
		return TRUE;
	}
}
