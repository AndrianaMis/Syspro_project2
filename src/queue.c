#include "../include/headers.h"

queue qu = { .front_in=0, .rear_in=0 , .count=0 , .capacity=10, .jobs=NULL, .lock = PTHREAD_MUTEX_INITIALIZER, .not_empty = PTHREAD_COND_INITIALIZER, .not_full = PTHREAD_COND_INITIALIZER};
int jobCounter = 1;  //Counter ολων των εργασίων (για να φτιάξουμε το job_xx)
int concurrencyLevel = 1; //αρχικό


//Αρχικοποίηση της κοινής ουράς 
void initQueue(int capacity) {  
    qu.capacity=capacity;
    qu.front_in = qu.rear_in = qu.count = 0;
    qu.jobs = (tripl*)malloc(qu.capacity * sizeof(tripl));
    printf("queue initialized of size %d \n", capacity);
    pthread_mutex_init(&qu.lock, NULL);
    pthread_cond_init(&qu.not_empty, NULL);
    pthread_cond_init(&qu.not_full, NULL);
}

//Προσθήκη στην ουρα. Παίρνει ως είσοδο μια τριπλέτα <job, jobID, client_socket> όπου το jobID είναι το μοναδικό
//αναγνωριστικό για την εργασια που ζητάει ο πελάτης, το job είναι η συγκεκριμένη εργασία, και
//το clientSocket είναι ο περιγραφέας αρχείου (file descriptor) για το socket που επιστρέφει η
//accept() για επικοινωνία με τον συγκεκριμένο πελάτη.
void add(tripl t){
    queue *q=&qu;
    pthread_mutex_lock(&q->lock);  //Πρόσβαση στους κοινούς πόρους
    while (q->count == q->capacity) {
        pthread_cond_wait(&q->not_full, &q->lock);            //περιμένει μέχρι να μην είναι full και κάνει release to mutex
    }

    q->jobs[q->rear_in].job = strdup(t.job);
    q->jobs[q->rear_in].jobID = strdup(t.jobID);
    q->jobs[q->rear_in].client_socket=t.client_socket;

    q->rear_in = (q->rear_in + 1) % q->capacity;
    q->count++;
    //printf("count is %d\n", qu.count);

    pthread_cond_signal(&q->not_empty);              //oxi adeia

    pthread_mutex_unlock(&q->lock);
}


//Διαγραφή και επιστροφή του πρώτου στοιχείου της ουράς 
tripl del(){
    queue *q=&qu;
    pthread_mutex_lock(&q->lock); //Πρ΄σοβαση τους κοινούς πόρους 
    while(q->count==0){
        printf("waiting\n");
        pthread_cond_wait(&q->not_empty , &q->lock);  //Περιμένει μέχρι να υπάρχει τουλάχιστον 1 job στην ουρά 

    }
    tripl t = q->jobs[q->front_in];
    q->front_in = (q->front_in + 1) % q->capacity;
    q->count--;
    pthread_cond_signal(&q->not_full);          //oxi gemath
    pthread_mutex_unlock(&q->lock); // Unlock the mutex

    return t;

    
}

//Εκτύπωση ουρας - debugging
void printq(){
    pthread_mutex_lock(&qu.lock);
    for(int i=qu.front_in; i< (qu.front_in + qu.count); i=(i+1)%(qu.capacity)){
            printf(" %d  <%s , %s, %d>\n",i, qu.jobs[i].job, qu.jobs[i].jobID, qu.jobs[i].client_socket);
    }
    pthread_mutex_unlock(&qu.lock);

}

//Συνάρτηση που αφαιρεί μια εργασία από μια συγκεκριμένη θέση index. Χρησιμοποιείται για την λειτουργία του stop job_xx command
void removeindex(int index){
    queue *q=&qu;
    pthread_mutex_lock(&q->lock);

    if(index<0 || index>= (q->front_in + q->count)){
        pthread_mutex_unlock(&q->lock);

        printf("invalid index\n");
        return;
    }

    for(int i=index; i< (q->count + q->front_in); i++){  //από τη θέση και μετά, μεταφέρει τα jobs και καλύπτει τη θέση index
        int current= index % q->capacity;
        int next=(current +1) % q->capacity;
        q->jobs[current]=q->jobs[next];
        printf(" %d  ->  %d \n", current, next);
    }
    q->count--;

    if(q->count==0){
        q->front_in=0;
    }    
    pthread_cond_signal(&q->not_full);          //oxi gemath

    pthread_mutex_unlock(&q->lock);



}


