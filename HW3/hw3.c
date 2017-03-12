#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define report_error(str) {printf("Error: %s - %s, line %d\n",(str), strerror(errno), __LINE__); exit(errno);}
#define rep_error(str,rc) {printf("Error: %s - %d, line %d\n",(str),(rc), __LINE__); exit(-1);}

#define MAX 4096

#define LOCK(lock,rc)\
	do {\
		if((rc = pthread_mutex_lock(&(lock)))!= 0)\
			rep_error("can't lock the list mutex",rc)\
	} while(0)

#define UNLOCK(lock,rc)\
	do {\
		if((rc = pthread_mutex_unlock(&(lock)))!= 0)\
			rep_error("can't unlock the list mutex",rc)\
	} while(0)

//---------------------Global-variables-for-the-simulator----------------------
int stop = 0;
int m_size, num_writers, num_readers, m_time;

// pthread_mutex_t glob_lock;
pthread_cond_t stop_cond;


typedef struct Node
{
	struct Node *next;
	struct Node *prev;
	int value;
	
}list_node;

typedef struct List
{
	int size;
	list_node *First;
	list_node *Last;
	pthread_mutex_t lock;
	pthread_cond_t cond_var;
	
}intlist;

void intlist_init(intlist* list);
void intlist_destroy(intlist* list);
pthread_mutex_t* intlist_get_mutex(intlist* list);
void intlist_push_head(intlist* list, int value);
int intlist_pop_tail(intlist* list);
void intlist_remove_last_k(intlist* list, int k);
int intlist_size(intlist* list);
void free_list_rec(list_node * node);

//-----------------for-main------------------
void *reader(void *list);
void *writer(void *list);
void *cut_list(void *list);
intlist *_list; //global list

/**
 * [intlist_init initialize the fileds of the struct intlist]
 * @param list != NULL [pointer to list]
 * @return void
 */
void intlist_init(intlist* list)
{
	int rc;
	list->size = 0;
	list->First = NULL;
	list->Last = NULL;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	if ((rc = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE)) != 0)
		rep_error("INIT: can't set type of the attr to recursive",rc)

	if((rc = pthread_mutex_init(&list->lock,&attr)) != 0)
		rep_error("INIT: can't init mutex for list",rc)
	// printf("%s\n","list mutex created succesfuly" );
	if((rc = pthread_cond_init(&list->cond_var,NULL)) != 0)
		rep_error("INIT: can't init condition variable",rc)
	pthread_mutexattr_destroy(&attr);
}

/**
 * [intlist_destroy destroy the fileds of the struct intlist]
 * @param list != NULL [pointer to initialized list]
 * @return void
 */
void intlist_destroy(intlist* list)
{
	int rc;
	LOCK(list->lock,rc);

	if(list->First != NULL)
		free_list_rec(list->First);

	UNLOCK(list->lock,rc);

	if((rc = pthread_mutex_destroy(&list->lock)) != 0)
		rep_error("DES: can't destroy the mutex lock of the list",rc)
	if((rc = pthread_cond_destroy(&list->cond_var)) != 0)
		rep_error("DES: can't destroy the condition variable of the list",rc)

}

/**
 * [intlist_get_mutex return the mutex of the list]
 * @param  list != NULL [pointer to initialized list]
 * @return  pthread_mutex_t*  [return pointer to the mutex of the list]
 */
pthread_mutex_t* intlist_get_mutex(intlist* list)
{
	return &list->lock;
}

/**
 * [intlist_push_head push node with the given value to the head of the list]
 * @param  list != NULL, value [pointer to initialized list, and vlaue]
 * @return void
 */
void intlist_push_head(intlist* list, int value)
{
	list_node *node = (list_node*) malloc(sizeof(list_node));
	node->value = value;
	node->prev = NULL;
	int rc;
	LOCK(list->lock,rc);

	node->next = list->First;
	if (list->First != NULL)
		list->First->prev = node;
	list->First = node;

	if (!list->size)
		list->Last = node;

	list->size++; // can be done with _sync_fetch_and_add for preformance
	
	if((rc = pthread_cond_signal(&list->cond_var)) != 0)
			rep_error("PUSH: can't signal this condition variable",rc)

	UNLOCK(list->lock,rc);
}

/**
 * [intlist_pop_tail pop node from the given list and return its value]
 * @param  list != NULL [pointer to initialized list]
 * @return value [the value of the node removed from the list]
 */
int intlist_pop_tail(intlist* list)
{
	int value,rc;
	list_node *temp;
	LOCK(list->lock,rc);

	while(list->size == 0) 
	{
		if((rc = pthread_cond_wait(&list->cond_var,&list->lock)) != 0)
			rep_error("POP: can't wait on this condition variable",rc)
	}
	temp = list->Last;
	value = list->Last->value;
	
	list->Last = temp->prev; //size != 0 (at least 1)
	if (list->size == 1)
		list->First = NULL;
	else
		list->Last->next = NULL;

	list->size--; // can be done with _sync_fetch_and_sub for preformance

	UNLOCK(list->lock,rc);
	free(temp);

	return value;
}

/**
 * [intlist_remove_last_k remove the last k nodes of the list]
 * @param  list != NULL ,k >= 0[pointer to initialized list, and nonnegative integer]
 * @return void
 */
void intlist_remove_last_k(intlist* list, int k)
{
	int i,j;
	list_node *temp;
	LOCK(list->lock,i);

	for (i=0; i<k; ++i)
	{
		if(list->size == 0) 
			break;
		temp = list->Last;
		list->Last = temp->prev; //size != 0 (at least 1)
		if (list->size == 1)
			list->First = NULL;
		else
			list->Last->next = NULL;
		list->size--;
		free(temp);
	}

	UNLOCK(list->lock,i);
}

/**
 * [intlist_size return the size of the list]
 * @param  list != NULL[pointer to initialized list]
 * @return int
 */
int intlist_size(intlist* list)
{
	return list->size;
}

void free_list_rec(list_node * node)
{
	if (node != NULL)
		free_list_rec(node->next);
	free(node);
}

//-----------------------------PART--II--------------------------------

void *reader(void *list)
{	
	int r;
	while(!stop)
	{
		// LOCK(glob_lock,r);
		intlist_pop_tail((intlist *)list);
		// UNLOCK(glob_lock,r);
	}
	printf("reader exit succesfuly\n");
	pthread_exit(NULL);
}

void *writer(void *list)
{
	int r;
	intlist *l =(intlist *)list;
	while(stop<1)
	{	

		LOCK(*intlist_get_mutex(l),r);
		if (intlist_size(l) > m_size) //need to get size when locked by mutex
		{
			if((r = pthread_cond_signal(&stop_cond)) != 0)
				rep_error("WRITER: can't signal this condition variable",r)
		}
		r  = rand()%MAX;
		intlist_push_head(l,r);
		UNLOCK(*intlist_get_mutex(l),r);
	}
	printf("writer exit succesfuly\n");
	pthread_exit(NULL);
}

void *cut_list(void *list)
{
	intlist *l = (intlist *)list;
	int rc;
	while(stop<2)
	{
		LOCK(*intlist_get_mutex(l),rc);
		while(intlist_size(l) < m_size && stop<2)
			if((rc = pthread_cond_wait(&stop_cond,intlist_get_mutex(l))) != 0)
				rep_error("GC: can't wait on this condition variable",rc)
		while(intlist_size(l) >= m_size)
		{
			rc = (int)(intlist_size(l) +1)/2;
			// printf("%d - items in list\n", intlist_size(l));
			intlist_remove_last_k(l, rc);
			printf("GC: %d items removed from the list\n",rc);
		}
		UNLOCK(*intlist_get_mutex(l),rc);
	}
	printf("GC exit succesfuly\n");
	return NULL;
}

void printl(intlist *list)
{
	int i = 0;
	printf("the elements in the list\n");
	while(intlist_size(list) != 0)
	{
		++i;
		printf("%d elem - %d\t",i,intlist_pop_tail(list));
	}
	printf("\n");
}

int main(int argc, char const *argv[])
{
	if (argc != 5)
	{
		printf("usage: simulator <WNUM> <RNUM> <MAX> <TIME>\n");
		exit(-1);
	}
	int i,rc;
	char *temp;
	num_writers = (int)strtol(argv[1], &temp, 10);
	if (*temp)
	{
		printf("can't convert WNUM to int using strtol, %s\n" ,argv[1]);
		exit(-1);
	}
	num_readers = (int)strtol(argv[2], &temp, 10);
	if (*temp)
	{
		printf("can't convert RNUM to int using strtol, %s\n" ,argv[2]);
		exit(-1);
	}
	m_size = (int)strtol(argv[3], &temp, 10);
	if (*temp)
	{
		printf("can't convert MAX to int using strtol, %s\n" ,argv[3]);
		exit(-1);
	}
	m_time = (int)strtol(argv[4], &temp, 10);
	if (*temp)
	{
		printf("can't convert TIME to int using strtol, %s\n" ,argv[4]);
		exit(-1);
	}
	//-----------------------------simulator-code--------------------------------
	
	srand(time(NULL));

	_list = (intlist *)malloc(sizeof(intlist));
	if(_list == NULL)
		report_error("MAIN: can't allocate memory for list")
	intlist_init(_list);


	pthread_t gc_thread;
	pthread_t writers[num_writers];
	pthread_t readers[num_readers];
	pthread_attr_t attr;

	if((rc = pthread_attr_init(&attr)) != 0)
		rep_error("MAIN: can't init attr",rc)
	// pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// if((rc = pthread_mutex_init(&glob_lock,NULL)) != 0)
	// 	rep_error("MAIN: can't init global mutex",rc);

	if((rc = pthread_cond_init(&stop_cond,NULL)) != 0)
		rep_error("MAIN: can't init global cond",rc)	


	if ((i = pthread_create(&gc_thread,&attr, cut_list,(void *)_list)) != 0)
		rep_error("MAIN: can't create the gc thread",i)

	for (i = 0; i < num_writers; ++i) // creat the writers
	{
		if((rc = pthread_create(&writers[i],&attr, writer,(void *)_list)) != 0)
			rep_error("MAIN: can't create writer thread",rc)
		printf("MAIN: writer no. %d created\n", i);
	
	}
	for (i = 0; i < num_readers; ++i) // create the readers
	{
		if((rc = pthread_create(&readers[i],&attr, reader,(void *)_list)) != 0)
			rep_error("MAIN: can't create writer thread",rc)
		printf("MAIN: reader no. %d created\n", i);
	}

	int t_time = m_time;
	while(t_time != 0)
	{
		printf("MAIN: time to end: %d seconds\n",t_time--);
		sleep(1);
	} // can be done by sleep(m_time)

	__sync_fetch_and_add(&stop,1);

	for (i = 0; i < num_readers; ++i) // create the readers
		if((rc = pthread_join(readers[i],NULL)) != 0)
			rep_error("MAIN: can't join on reader thread",i)

	__sync_fetch_and_add(&stop,1);

	for (i = 0; i < num_writers; ++i) // creat the writers
		if((rc = pthread_join(writers[i],NULL)) != 0)
			rep_error("MAIN: can't join on writer thread",i)

	__sync_fetch_and_add(&stop,1);

	if((i = pthread_cond_signal(&stop_cond)) != 0)
		rep_error("MAIN: can't signal this condition variable",i)

	if((rc = pthread_join(gc_thread,NULL)) != 0)
		rep_error("MAIN: can't join on gc thread",i)

	printf("MAIN: the size of the list is %d\n", intlist_size(_list));
	printl(_list);

	printf("%s\n", "MAIN: Now Cleaning");

	//Clean up and exit
	intlist_destroy(_list);
	free(_list);
	if((rc = pthread_attr_destroy(&attr)) != 0)
		rep_error("MAIN: can't destroy attr",rc);
	// if((rc = pthread_mutex_destroy(&glob_lock)) != 0)
	// 	rep_error("MAIN: can't destroy attr",rc);
	if((rc = pthread_cond_destroy(&stop_cond)) != 0)
		rep_error("MAIN: can't destroy attr",rc);
	printf("Done\n");
	exit(0);
}
