#include "../include/headers.h"

//Βοηθητική συνάρτηση για να στέλνει το μήνυμα "END" μετά από εκτέλεση κάθε εντολής ώστε ο jobCommander να τελειώσει
void write_ending(int socket){
    sleep(1);
    char* ending=malloc(sizeof(char*));

    strcpy(ending, "END");
    write(socket, ending, strlen(ending));
    free(ending);
    close(socket);
    printf("\nSENT ENDING MESSAGE!\n\n");
}


//Συνάρτηση που καθαρίζει την ουρά.
// Χρησιμοποιείται αν λάβει την εντολή exit και πρέπει να καθαρίσει την ουρά που περιέχει τις εντολές προς εκτέλεση
void cleanq() {
    pthread_mutex_lock(&qu.lock);               //Lock το mutex της ουράς για synchronization (shared buffer)
    for (int i = 0; i < (qu.count); i++) {
        write(qu.jobs[i].client_socket, "SERVER TERMINATED BEFORE EXECUTION", strlen("SERVER TERMINATED BEFORE EXECUTION")); //Στέλνω μήνυμα σε κάθε client που περιμένει να εκτελε΄στεί η εργασία που έστειλε
        free(qu.jobs[i].job);
        free(qu.jobs[i].jobID);
    }
    free(qu.jobs);
    pthread_mutex_unlock(&qu.lock);
  
    printf("queue cleaned!\n");
    return;
}


int receiving_flag=1;              //flag που χρησιμοποιείται για έλεγχο πριν τo accept και την δημιουργία controller threads.
                                    //Χρησιμοποιείται ώστε να μην δέχεται ο server άλλα connection όταν δεχτεί την εντολή exit
int actives=0;               //active worker threads



//
void* workers_f(void *arg){
while(1){                                 //shmantiko

    pthread_mutex_lock(&qu.lock);
    while(qu.count==0){                        //Κάθε worker thread ξυπνάει όταν υπάρχει τουλάχιστον μια εργασία στην κοινόχρηστη ουρά
        pthread_cond_wait(&qu.not_empty, &qu.lock);            //δηλαή πρειμένει μέχρι να γίνει singal η cond var not_empty ππου έχει η ουρά
    } 
    printf("in worker\n");

    //To concurrencyLevel μας λεει ποσα απο αυτα τα worker threads μπορουν να τρεχουν εργασιες ταυτοχρονα.
    while(concurrencyLevel<=actives){           //actives είναι τα active worker threads = jobs running 
        usleep(1000);                      //όσο οι εργασίες που τρέχουν είναι >= (=) με το concurrency , περιμένουμε μέχρι κάποια εργασία να τελείωσει
    }
    //printf("stop wait\n");
    pthread_mutex_unlock(&qu.lock);   
 
    tripl to_execute=del();     //διαγράφουμε την εργασία - τριπλέτα που θα εκτελεστεί από την ουρά
    //printf("deleted\n");
    //printf("qu count is :%d\n", qu.count);
    char* job_to_execute=to_execute.job;

    actives++;


    /*if(write(to_execute.client_socket, "hello", strlen("hello"))<0){
        perror("again error");
        exit(1);
    }*/
    
    
    char* copy=strdup(job_to_execute);                //δημιουργώ ένα αντίγραφο για να μετρήσω τις λέξεις της εργασίας
    int argcount=0;
    char *ttoken=strtok(copy, " ");
    while (ttoken != NULL) {
        argcount++;
        ttoken = strtok(NULL, " ");
    }
    char **args = (char **)malloc((argcount + 1) * sizeof(char *));          //memory allocation για τα args
    free(copy);

    copy=strdup(job_to_execute);                                     //παλι αντίγραφο για να φτιάξω τον πίνακα με τις λέξεις
    ttoken=strtok(copy, " ");
    for (int i = 0; i < argcount; i++) {
        args[i] = strdup(ttoken);
        if (args[i] == NULL) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        ttoken = strtok(NULL, " ");

    }
    args[argcount] = NULL;
   // for(int i=0; i<argcount; i++){
     //   printf("%s\t", args[i]);
    //}

   int pid=fork();     //fork ένα παιδί για την εκτέλεση της εργασίας 
    if(pid==-1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if(pid==0){
        //paidi
        //printf("paidi\n");
        char filename[256];
        sprintf(filename, "%d.txt", getpid());   //φτιάχνουμε το file pid.txt 
        
        int  fd;
        mode_t fdmode = S_IRUSR|S_IWUSR|S_IRGRP| S_IROTH;
        //printf("filename %s\n", filename);

        if((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, fdmode))<0){
            perror("failed file creation for server\n");
            exit(EXIT_FAILURE);
        }

        //printf("fd=%d \n", fd);
        if (dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {     //output redirection στο αρχείο που μόλις φτιάξαμε
            perror("dup2");
            close(fd);
            exit(EXIT_FAILURE);
        }        

        close(fd);

        if (execvp(args[0], args) == -1) {              //execution
            perror("execvp");
        }

        exit(EXIT_SUCCESS);

    }
    else{

        int status;
        if (waitpid(pid, &status, 0) == -1) {      //ο γονέας περιμένει το παιδί να τελείωσει 
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        //printf("parent\n");
        
        char filename[256];

        sprintf(filename, "%d.txt", pid);
        //printf("parent filename %s\n", filename);

        char start[200];

        snprintf(start, sizeof(start), "\n-------- %s output start --------\n", to_execute.jobID);  //για λόγους ευκρίνειας

    
       // sleep(1);

        if((write(to_execute.client_socket, start, strlen(start)))<0){
            printf("csock:%d\n", to_execute.client_socket);
            perror("errorr writing1\n");
            exit(EXIT_FAILURE);
        }


        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));

        int ffd=open(filename, O_RDONLY);   //ανοίγουμε το file που έφτιαξε το παιδί για να διαβάζουμε το output 
        if(ffd<0){
            perror("pid file openning\n");
            exit(EXIT_FAILURE);
        }
        read(ffd, buffer, sizeof(buffer));

        //printf("buffer: %s\n", buffer);
        close(ffd);

        sleep(1);
        if((write(to_execute.client_socket, buffer, strlen(buffer)))<0){    //γράφουμε το output 
            printf("error writing2\n");
            exit(1);
        }
        fflush(stdout);
        
        char end[200];
        snprintf(end, sizeof(end), "\n-------- %s output end --------\n", to_execute.jobID); //για λόγους ευκρίνειας
        if((write(to_execute.client_socket, end, strlen(end)))<0){
            perror("failes 3");
            exit(1);
        }

    
        write_ending(to_execute.client_socket);  //μήνυμα τέλους





        remove(filename);
    }
    //printf("fork done\n");
    actives--;
    
}


}

//Το κάθε controller thread θα διαβάζει από τη σύνδεση την εντολή του πελάτη jobCommander και θα εκτελεί την εντολή
//Η συνάρτηση του κάθε controller thread παρίνει ως είσοδο το client και το server socket.

//Το κάθε controller thread θα διαβάζει από τη σύνδεση την εντολή του
//πελάτη jobCommander και θα εκτελεί την εντολή ή θα εισάγει στον κοινόχρηστο buffer την
//εργασία για εκτέλεση
void* controllert_f(void* arg){
    int csock=((contr_input*) arg)->client_socket;
    int sersock=((contr_input*)arg)->server_socket;

    //printf("\n client socket: %d\n", csock);
    char buf[1024];
    int b;
    memset(buf, 0, sizeof(buf));
    b = read(csock, buf,1024 - 1); 
    if(b<0){
        perror("reading");
    }
    buf[b] = '\0';


   // printf("received :%s\n", buf);
    //printf("read '%s'\n", buf);
    //Χωρίζουμε την ετνολή για να βρούμε τον τύπο
    char type[256];
    char command[1024];
    char * token=strtok((char*)buf, " ");
    
    if(token!=NULL){
        strcpy(type, token);     //ο τύπος της εντολής που δέχτηκε (issueJob , poll, stop, exit, setConcurrency)
        token=strtok(NULL, "\0");
    }
    if(token!=NULL){
        strcpy(command, token);        //το actual job (αν υπάρχει)
    }

/*ένα controller thread πρέπει να μπλοκάρεται και να περιμένει όταν ο
ενταμιευτής είναι γεμάτος*/
    pthread_mutex_lock(&qu.lock);

    if(qu.count==qu.capacity){
        pthread_cond_wait(&qu.not_full, &qu.lock);

    }
    pthread_mutex_unlock(&qu.lock);




//Έλεγχος για το ποιός είναι ο τύπος της εντολής που δέχτηκε
    if(strcmp(type, "issueJob")==0){
        /*Το controller thread θα δημιουργεί ένα μοναδικό jobID για την εργασία που ζητάει ο πελάτης να
        τρέξει και θα τοποθετεί σε έναν ενταμιευτή συγκεκριμένου μεγέθους (που ορίζεται από το
        bufferSize) την τριπλετα <jobID, job, clientSocket>, όπου το jobID είναι το μοναδικό
        αναγνωριστικό για την εργασια που ζητάει ο πελάτης, το job είναι η συγκεκριμένη εργασία, και
        το clientSocket είναι ο περιγραφέας αρχείου (file descriptor) για το socket που επιστρέφει η
        accept() για επικοινωνία με τον συγκεκριμένο πελάτη. Στη συνέχεια το controller thread
        επιστρέφει στον πελάτη το μήνυμα:
        JOB <jobID, job> SUBMITTED*/
        tripl to_add;            //Η τριπλέτα που θα μπεί στο shared queue
        to_add.job=strdup(command);
        to_add.jobID=malloc(100);
        to_add.client_socket=csock;
        snprintf(to_add.jobID, 10, "job_%d", jobCounter);         //Φτιάχνω το job_XX
        jobCounter++;
        add(to_add);
        char response[256];
        sprintf(response, "JOB <%s , %s> SUBMITTED", to_add.jobID, to_add.job );       //στέλνουμε στον jobCommander το μήνυμα "JOB <job_XX, job> SUBMITTED"
        write(csock, response, strlen(response) );
        //printf("qu count is now: %d\n", qu.count);
        //printq();




    }
    else if(strcmp(type, "setConcurrency")==0){
        /*Το controller thread ενημερώνει την τιμή μιας κοινόχρηστης μεταβλητής concurrencyLevel που
        αρχικά έχει τιμή 1.
        Στέλνει στον πελάτη το μήνυμα
        CONCURRENCY SET AT Ν*/
        int new_level=atoi(command);
        concurrencyLevel=new_level;
        char response[100];
        sprintf(response, "CONCURRENCY SET AT %d", concurrencyLevel);   //Στέλνουμε στον jobCommander το μήνυμα "CONCURRENCY SET AT X"
        write(csock, response, strlen(response));

        write_ending(csock);               //Στέλνουμε στον jobCommander το μήνυμα end ώστε να κάνει exit
        
    }
    else if(strcmp(type, "stop")==0){
        /*Αν το αίτημα είναι stop <jobID>, το controller thread αφαιρεί το αντίστοιχο job από την
        κοινόχρηστη ουρά και στέλνει στον πελάτη το μήνυμα
        JOB <jobID> REMOVED
        ή
        JOB <jobID> NOTFOUND οπως περιγράφηκε ανωτέρω.
        */
       int found=0; //flag

        pthread_mutex_lock(&qu.lock);     //κάνει lock το mutex της ου΄ρας για να κάνει access στα δεδομένα της 

       for(int i=qu.front_in; i<(qu.front_in+qu.count); i++){     //ψάχνει στην ου΄ρα αν υπάρχει μέσα η εργασία με jobID
        if(strcmp(qu.jobs[i].jobID, command)==0){
            found=1;
            removeindex(i);
            char response[1100];
            sprintf(response, "JOB %s REMOVED", qu.jobs[i].jobID);  //Αν την βρει , την διαγράφει και στέλνει "JOB JOBID REMOVED"
            write(csock, response, strlen(response));

        }

        pthread_mutex_unlock(&qu.lock);

       }
       if(!found){
        char response[1100];
        sprintf(response, "JOB %s NOT FOUND", command);  //Αλλίως στέλνει "JOB JOBID NOT FOUND"
        write(csock, response, strlen(response));       
       }

       write_ending(csock);             //Τέλος στέλνουμε το "END" για να κάνει ο jobCommander exit
    }
   else if(strcmp(type, "poll")==0){
        /*Το controller thread διατρέχει τον κοινόχρηστο ενταμιευτή, μαζεύει τα ζεύγη <jobID, job> των
        εντολών του συγκεκριμένου πελάτη, και τα επιστρέφει στον πελάτη σε μήνυμα.*/
        char response[1024] = "";
        int response_len = 0;

        pthread_mutex_lock(&qu.lock);    //κάνει lock το mutex της ου΄ρας για να κάνει access στα δεδομένα της 
        if(qu.count>0){                  //den einai adeia
            for(int i=qu.front_in; i<(qu.front_in+qu.count); i++){    // ψάχνει την ουρά για jobs που έχει στείλει ο συγκεκριμένος client για να τα εμφανίσει μαζεμένες
                if(qu.jobs[i].client_socket==csock){    ///να έχουν το ίδιο client socket

                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "<%s, %s>\n", qu.jobs[i].jobID, qu.jobs[i].job);
                    size_t buffer_len = strlen(buffer);

                    if (response_len + buffer_len < sizeof(response)) {    //Αυξάνουμε το μέγεθος της απάντησης κάθε φορά που βρ΄ίσκουμε ένα job στην ουρά
                        strcat(response, buffer);
                        response_len += buffer_len;
                    } else {
                        break; 
                    }
                }
            }

        }

        pthread_mutex_unlock(&qu.lock);
        if(response_len>0){     // Αν βρέθηκε έστω μια εργασία στην ουρά
            write(csock, response, strlen(response));    //Γράφουμε τις εργασίες  (μαζεμένες)
        }
        else{
            write(csock, "No jobs in the buffer", strlen("No jobs in the buffer"));
        }

        write_ending(csock);

    }
    else if(strcmp(type, "exit")==0){
        /*Το controller thread τερματίζει τον jobExecutorServer. Ο τερματισμός του jobExecutorServer
        θα πραγματοποιηθεί μετά την ολοκλήρωση όλων των εργασιών που τρέχουν και την
        επιστροφή των εξόδων τους στον αντίστοιχο πελάτη Ο ενταμιευτής αδειάζει χωρίς να τρέξουν
        οι εργασίες που περιέχει και οι πελάτες που έχουν υποβαλει τις εργασίες θα ενημερώνονται με
        μήνυμα
        SERVER TERMINATED BEFORE EXECUTION*/
        receiving_flag=0;       //Το receiving flag γίνεται 0 για να μην δέχεται ο server άλλες συνδέσεις από clients
        cleanq();           //Καθαρίζουμε την ου΄ρα από τις εργασίες που είναι σε αναμονή για execution
        while(actives>0){      //Όσο υπάρχουν active worker threads δηλαδή δουλειές που τρέχουν εκείνη τη στιγμη 
            usleep(1000);     //περιμένουμε μέχρι actives==0
        }
        printf("active jobs finished!\n");
        usleep(100);
        write(csock,"SERVER TERMINATED BEFORE EXECUTION", strlen("SERVER TERMINATED BEFORE EXECUTION") );   //Γράφουμε και στον πελάτη που έστειλε το exit
        usleep(100);
        close(csock);             //κλείσιμο client socket
        close(sersock);           //κλείσιμο server socket
        exit(EXIT_SUCCESS);




    }
    

    //close(csock);
}


//main thread function. Παίρνει ως είσοδο το portnum και το threadpoolsize για να κάνει accept συνδέσεις και ν αδημιουργήσει και τα worker threads

void* maint_f(void* arg){

    int portnum=((main_input*) arg)->port;
    int threadpoolsize=((main_input*) arg)->pool;



    //Το αρχικό main thread θα δημιουργεί #threadPoolSize worker threads.
    workers=(pthread_t*)malloc(threadpoolsize * sizeof(pthread_t)); 
    for(int i=0; i<threadpoolsize; i++){
                                        
        pthread_create(&workers[i], NULL, workers_f, NULL);
    }



    //Θα ακούει για συνδέσεις από jobCommander πελάτες
    struct sockaddr_in serv_addr;
    struct sockaddr *servaddr = (struct sockaddr*)&serv_addr; 

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port=htons(portnum);


    int sockk ,csock;
    if((sockk=socket(AF_INET, SOCK_STREAM, 0))<0){       //Δημιουργία client socket
        perror("sockk creation");
        exit(EXIT_FAILURE);
    }
    int opt=1;
    if (setsockopt(sockk, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if((bind(sockk, servaddr, sizeof(serv_addr)))<0){
        perror("bind failed");
        close(sockk);
        exit(EXIT_FAILURE);
    }


    if (listen(sockk, 5) < 0) {
        perror("Listen failed");
        close(sockk);
        exit(EXIT_FAILURE);
    }
    printf("Listening for connections to port %d\n", portnum);

    socklen_t clientlen;

    struct sockaddr_in client_addr;              //client address 
    struct sockaddr *clientaddr = (struct sockaddr*)&client_addr; 
    struct hostent *rem;

    while(1){
        clientlen=sizeof(client_addr); 
        printf("client %d\n", clientlen);       

        if(receiving_flag){                       //αν είναι σε θέση να δέχεται connections. 
                                    //Το receiving_flag γίνεται 0 μόνο όταν δέχεται ο server από κάποιον commander την εντολή exit

             //otan enas client connects
            if((csock=accept(sockk, clientaddr, &clientlen) )<0){
                perror("accept");
                continue;
            }
            if ((rem = gethostbyaddr((char *) &client_addr.sin_addr.s_addr,sizeof(client_addr.sin_addr.s_addr), client_addr.sin_family))== NULL) {
                herror("gethostbyaddr"); exit(1);
            }
            printf("Accepted connection from %s\n", rem->h_name);

            //. Όταν συνδέεται ένας jobCommander πελάτης το main thread θα δημιουργεί ένα controller thread.
            pthread_t controller;
            contr_input* contr=malloc(sizeof(contr_input));      //struct που θα πα΄ρινει το κάθε controller thread ως είσοδο με τα δεδομένα που χρειάζεται
            contr->client_socket=csock;
            contr->server_socket=sockk;
            pthread_create(&controller, NULL, controllert_f , contr  );

            sleep(1);
            if (pthread_detach(controller) != 0) {              //detaching το κάθε controller thread (δεν κάνω join)
                perror("pthread_detach");
                if (pthread_join(controller, NULL) != 0) {
                    perror("pthread_join");
                }
            }
            

            free(contr);

        }
       
    }
    close(sockk);

    

}

//Ο multi-threaded server πα΄ρινει ως ορίσματα από τη γραμμή ετντολ΄νω: [portnum] [buffersize] [threadpoolsize]
//ο jobExecutorServer δημιουργεί 3 είδη threads : main_thread, controller_threads , worker_threads
int main(int argc, char** argv){
    if(argc<4){
        printf("Not enough arguments!\n");
        exit(EXIT_FAILURE);
    }


    int portnum=atoi(argv[1]);            //portnum
    int buffersize=atoi(argv[2]);      //buffersize (μέγεθος κοινού ενταμιευτή)
    int threadpoolsize=atoi(argv[3]);        //threadpoolsize (πόσα worker threads θα δηημιουργηθούν για να έιναι έτοιμα προς χρήση)

    

    if(buffersize>0) initQueue(buffersize);     //αρχικοποίηση κοινού ενταμιευτή (κοινής ουράς)
    else{
        printf("buffersize must be greater than 0!!!\n");
        return 0;
    }



    main_input* input=malloc(sizeof(main_input));    //struct που θα πα΄ιρνει το main_thread ως είσοδο
    input->pool=threadpoolsize;                   
    input->port=portnum;
    
    //To main thread
    pthread_t main_t;
    pthread_create(&main_t, NULL, maint_f, input );

    //Θα περιμένουμε να τελείωσει
    pthread_join(main_t, NULL);
    free(input);


    return 0;
}