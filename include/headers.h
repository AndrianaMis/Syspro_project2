#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h> /* for hton * */



//---------------------------------------------QUEUE IMPLEMENTATION------------------------------------------------------//

struct tripleta{
    char* jobID;                       //job_XX
    char *job;                          //actual job
    int client_socket;

};
typedef struct tripleta tripl;

//Μια θα είναι η ουρά και θα έχει προκαθορισμένο μέγεθος!
//θα είναι shared
struct Queue{
    int front_in , rear_in, count, capacity;
    tripl* jobs;                                       //array apo tripletes
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} ;
typedef struct Queue queue;

//globals
extern queue qu;           //κοινόχρηστη ουρά 
extern int jobCounter;     //counter όλων των jobs που έχουν περάσει από τον server, χρησιμεύει για job_XX 
extern int concurrencyLevel;    //concurrency
extern int actives;      //ενεργά workers


//The queue functions
void initQueue(int);
void add(tripl);
tripl del();
void printq();
void removeindex(int);
void cleanq();

//-----------------------------------------------------------------------------------------------------------------------//
   

pthread_t * workers;
typedef struct{
    int port;
    int pool;

} main_input;   //input για τη συνάρτηση του main thread

typedef struct{
    int client_socket;
    int server_socket;
} contr_input;  //input για τη συνάρτηση κάθε controller thread





