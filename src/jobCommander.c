#include "../include/headers.h"



/*Ο jobCommander δίνει τη δυνατότητα στον χρήστη να αλληλεπιδράσει με τον
jobExecutorServer μέσω απλών εντολών (commands). Οι εντολές δίνονται ως ορίσματα κατά
την κλήση του jobCommander και στέλνονται μέσω διαδικτύου στον jobExecutorServer.
Συγκεκριμένα, ο jobCommander θα παίρνει τα ακόλουθα ορίσματα:  [serverName] [portnum] [jobCommanderInputCommand]
*/
int main(int argc, char **argv){

    if(argc<4){
        printf("Give some arguments!\n");
        exit(EXIT_FAILURE);
    } 

    char* name=argv[1];               //serverName e.g. linux01.di.uoa.gr
    char *num=argv[2];               //portnum 
    int port=atoi(num);

    int buffer_size = 0;                         //υπολογισμός του μεγέθους της εντολής
    for (int i = 3; i < argc; i++) {
        buffer_size += strlen(argv[i]) + 1; 
    }

    char *command = malloc(buffer_size);
    if (command == NULL) {
        perror("malloc");
        return 1;
    }
    
    command[0] = '\0'; 
    for (int i = 3; i < argc; i++) {
        strcat(command, argv[i]);                                //command
        if (i < argc - 1) {
            strcat(command, " ");
        }
    }


    struct hostent *host;                                 
    if((host=gethostbyname(name))==NULL){                                 //hostname σε διεύθυνση IP 
        printf("Could not resolve name: %s\n", name);
    }


    //Αρχή επικοινωνιας 
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM,0)) == -1)          //Δημιουργία socket για network communication
        perror("Failed to create socket");

    struct sockaddr_in server_addr;
    struct sockaddr *serverptr = (struct sockaddr*)&server_addr;


    //αρχικοποίηση server address (sockaddr_in structure)
    memset(&server_addr, 0, sizeof(server_addr));                // Zero out the structure
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    memcpy(&server_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);


    if ((connect(sock, serverptr, sizeof(server_addr))) < 0) {                    //Σύνδεση socket με server address
        perror("connect");
        close(sock);
        free(command);
        return 1;
    }
    else{
        printf("connected\n\n");
    }

    fflush(stdout);
    int i=write(sock, command, strlen(command));                        //στέλνουμε ολόκληρη την εντολή μέσω του socket στον server
    if(i<0){
        perror("send");
        close(sock);;
        free(command);
        return -1;
    }
   // printf("sent %s\n", command);
    sleep(1);


    char buffer[2048];
    bzero(buffer, 2047);

   while(1){                                     //Loop για να διαβάζει ότι του στέλνει ο server ως απάντηση στην εντολή που έστειλε.
                                                //π.χ. αν έστειλε εντολή με issueJob, ο server θα επιστρέψει περισσότερα από 1 μηνύματα
                                                //JOB <jobID, job> SUBMITTED και το αποτέλεσμα της εκτέλεσης της εντολής
        
        //printf("reeading...\n");
        memset(buffer, 0, sizeof(buffer));

        int bytes_received = read(sock, buffer, sizeof(buffer) -1);       //διάβασμα 
        if (bytes_received < 0) {
            perror("read");
            close(sock);
            exit(EXIT_FAILURE);
        
        }else if(strlen(buffer)>0) {                               //αν έλαβε κάτι
            buffer[bytes_received] = '\0';

 //Ο jobExecutorServer στένει μετά από κάθε εκτέλεση "END" ή "SERVER TERMINATED BEFORE EXECUTION" (το οποόιο θα εκτυπώσει)
            if (strcmp(buffer, "END") == 0) {            
                //printf("Received 'END' message\n");
                break; // Exit the loop if "END" message is received
            }            // Process received data
            printf("%s\n", buffer);
            if(strcmp(buffer, "SERVER TERMINATED BEFORE EXECUTION")==0){
                break;
            }
        }
        

   }
    
    close(sock);
    free(command);

    return 0;
}
