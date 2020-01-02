/*sum.c
 *
 * CS 470 Project 1 (Pthreads)
 * Serial version
 *
 * Compile with --std=c99
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
// aggregate variables
typedef struct task_queue{
	int num;
	struct task_queue* next;
}task_queue;
pthread_mutex_t createlock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t masterlock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t global_val_lock=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wakeup=PTHREAD_COND_INITIALIZER;
pthread_cond_t onemoreidle=PTHREAD_COND_INITIALIZER;
pthread_cond_t worker_init_clear=PTHREAD_COND_INITIALIZER;
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
bool done = false;
int idle_thread = 0;

task_queue *tqhead,*tqtail;
// function prototypes
void* update(void*);
void q_init(task_queue**,task_queue**);
long q_dequeue(task_queue *,task_queue**);
void q_enqueue(task_queue *,task_queue**,long);
long q_size(task_queue *);
bool q_isempty(task_queue *);
long q_front(task_queue *);

/*
 * intialize queue
 */
void q_init(task_queue** head,task_queue** tail){
    (*head)=(task_queue*)malloc(1*sizeof(task_queue));
    (*head)->num=0;
    (*head)->next=NULL;
    (*tail)=NULL;
}
/*
 * return head val(first input number)
 */
long q_front(task_queue *head){
    if(q_isempty(head)){
	return -1;
    }else{
	return head->next->num;
    }
}
/*
 * if queue is empty, return 1 else 0
 */
bool q_isempty(task_queue *head){
    if(q_size(head)==0)
	return true;
    return false;
}
/*
 * return the queue size
 */
long q_size(task_queue *head){
    return head->num;
}
/*
 * insert the number in task queue
 */
void q_enqueue(task_queue* head,task_queue** tail, long num){
    task_queue* tmp;
    head->num+=1;
    tmp=(task_queue*)malloc(1*sizeof(task_queue));
    tmp->num=num;
    tmp->next=NULL;
    if(*tail){
	(*tail)->next=tmp;
	(*tail)=(*tail)->next;
    }else{
	head->next=tmp;
	(*tail)=tmp;
    }
}
/*
 * return the first node number in task queue
 */
long q_dequeue(task_queue* head,task_queue** tail){
    long number;
    task_queue* temp;
    number=q_front(head);
    if(number==-1) return -1;
    temp=head->next;
    head->next=temp->next;
    head->num-=1;
    if(head->next==NULL) (*tail)=NULL;
    if(temp)free(temp);
    return number;
}
/*
 * update global aggregate variables given a number
 */
void* update(void* a)
{
    long int number;
    pthread_mutex_lock(&masterlock);
    pthread_cond_signal(&worker_init_clear);
    do{
	while(q_isempty(tqhead)&& !done){
		idle_thread+=1;
		pthread_cond_wait(&wakeup,&masterlock);
		idle_thread-=1;
	}
	if(done){
		break;
	}
	number=q_dequeue(tqhead,&tqtail);
	pthread_mutex_unlock(&masterlock);
	// simulate computation
	sleep(number);
	// update aggregate variables
	pthread_mutex_lock(&global_val_lock);
	sum += number;
	if (number % 2 == 1) {
	    odd++;
	}
	if (number < min) {
	    min = number;
	}
	if (number > max) {
	    max = number;
	}
	pthread_mutex_unlock(&global_val_lock);
	pthread_mutex_lock(&masterlock);
	pthread_cond_signal(&onemoreidle);
    }while(1);
    pthread_mutex_unlock(&masterlock);
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    int no_thread;
    pthread_t* tid;
    int err,i,status;
    
    // check and parse command line options
    if (argc != 3) {
        printf("Usage: sum <infile> <number_of_thread>\n");
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];
    no_thread=(int)atoi(argv[2]);
    // load numbers and add them to the queue
    FILE* fin = fopen(fn, "r");
    char action;
    long num;
    tid=(pthread_t*)malloc((no_thread+1)*sizeof(pthread_t));
    q_init(&tqhead,&tqtail);
       idle_thread=0;
    for(i=0;i<no_thread;i++){
    	pthread_mutex_lock(&masterlock);
	err=pthread_create(&tid[i],NULL,update,0);
	if(err!=0){
		printf("ERROR: Pthread Create Error\n");
		exit(EXIT_FAILURE);
	}
	pthread_cond_wait(&worker_init_clear,&masterlock);
    	pthread_mutex_unlock(&masterlock);
    }
    while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
        if (action == 'p') {            // process
	    pthread_mutex_lock(&masterlock);
	    q_enqueue(tqhead,&tqtail,num);
	    pthread_cond_signal(&wakeup);
	    pthread_mutex_unlock(&masterlock);
        } else if (action == 'w') {     // wait
            sleep(num);
        } else {
            printf("ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fin);
    pthread_mutex_lock(&masterlock);
    while(idle_thread<no_thread || !q_isempty(tqhead)){
	pthread_cond_wait(&onemoreidle,&masterlock);
    }
    done=true; 
    pthread_cond_broadcast(&wakeup);
    if(tqhead)free(tqhead);
    pthread_mutex_unlock(&masterlock);
    for(int j=0;j<no_thread;j++){
	err=pthread_join(tid[j],(void**)&status);
    }
    // print results
    printf("%ld %ld %ld %ld\n", sum, odd, min, max);
    pthread_cond_destroy(&wakeup);
    pthread_cond_destroy(&worker_init_clear);
    pthread_cond_destroy(&onemoreidle);
    pthread_mutex_destroy(&masterlock);
    pthread_mutex_destroy(&createlock);
    pthread_mutex_destroy(&global_val_lock);
    // clean up and return
    return (EXIT_SUCCESS);
}

