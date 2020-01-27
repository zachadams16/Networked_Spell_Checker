#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
 
#include<pthread.h> //for threading , link with lpthread
 
#define MAX 1024
#define DEFAULT_PORT 8080

#include "load_dictionary.h"

typedef struct{

    char **log_buf;
    int *job_buf;
    int job_len,log_len; //queue sizes
    int job_count,log_count;
    int job_rear;
    int job_front;
    int log_front;
    int log_rear;
    int count;

}buf;

static buf buffer;

void *worker_handler(void *);
void buf_init();
void buf_insert_log();
void buf_insert_job();
void buf_remove_log();
void buf_remove_job();
extern node *hashTable[SIZE];
int search_word();
const size_t NUM_THREADS = 20;

int done  = 0; //count of the number of finsihed threads

pthread_mutex_t mutex;
pthread_cond_t full;
pthread_cond_t empty;
FILE *f;


int main(int argc , char *argv[]){
	
	f = fopen("lab3.log", "w+");
	pthread_mutex_init(&mutex, NULL); //initializing mutex and CV's
  	pthread_cond_init (&full, NULL);
  	pthread_cond_init (&empty, NULL);


    pthread_t threads[NUM_THREADS];

	static char DEFAULT_DICTIONARY[] = "words.txt"; //the default dictionary if the user doesnt enter one
	char *dictionary;
	dictionary = (char*)malloc(sizeof(dictionary));
    buffer.log_buf = (char **)malloc(5*sizeof(char*)); 
	int done = 1;
	int port;
	if(argc == 1){ // if the user doesnt enter anything
		port = DEFAULT_PORT;
		strcpy(dictionary, DEFAULT_DICTIONARY);
	}else{ //if the user enters a custom port and dictionary
		port = atol(argv[1]);
		strcpy(dictionary, argv[2]);
	}



		if(load(dictionary) == 0){ //run load_dictionary.h to load dictionary into memory
			printf("Dictionary did not load succesfully, \nEither you entered an invalid Dictionary or it is not in this folder. \n");
		}else{
			done = 0;
             printf("hi\n");
		
		}
	printf("port is - %d", port);

    int socket_desc , new_socket , c , *new_sock;
    struct sockaddr_in server , client;
    char *message;
    buf_init(&buffer,10,10); //initializing size of queue lengths

     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        puts("bind failed");
        return 1;
    }

    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        //Reply to the client
        message = "Hello Client , I have received your connection. And now I will assign a handler for you\n";
        write(new_socket , message , strlen(message));
         
        pthread_t threads;
        new_sock = malloc(1);
        *new_sock = new_socket;
         
            if( pthread_create( &threads , NULL ,  worker_handler , (void*) new_sock) < 0)
            {
                perror("could not create thread");
                return 1;
            }

        puts("Handler assigned");

    }
     
    if (new_socket<0)
    {
        perror("accept failed");
        return 1;
    }
    fclose(f);
    return 0;
}

int search_word(char *inputed_word,int length){

    unsigned int key = 0;
    struct node** iter;
    for(int i =0;i<length;i++){ //find hash of entered word
            key = (key * inputed_word[i] + inputed_word[i] + i)%SIZE;
        }
        iter = &hashTable[key];
        struct node* curr = *iter;
    while(curr){ //iterate through location in hash table to find the word
        if((strncmp(inputed_word,curr->word,length)==0))
            return 1;
        else{
            curr = curr->next;
        }
    }

    return 0;

}

void *worker_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int count = 0;
    int read_size;
    char *message, *client_message; 
    char *prompt = "\nenter another word\n";
    char word[MAX];

    message= (char*)malloc(sizeof(message));
    //Send some messages to the client
    message = "Greetings! I am your connection handler\n";
    write(sock , message , strlen(message));
     
    message = "Now type something for me to spellcheck (Enter \"quit1\"  to close the program) \n";
    write(sock , message , strlen(message));
     
    //Receive a message from client
    while( (read_size = recv(sock , word , MAX , 0)) > 0 )
    {
        client_message= (char*)malloc(2*sizeof(char));
        if(read_size <= 0)
            break;
        else {
            word[read_size-2] = '\0';    
            int check = search_word(word,read_size-2);

            //will close the program
            if(strncmp(word,"quit1",5)==0){ // to quit the program
                exit(0);
            }
            strcpy(client_message,word); 
             if(check == 1){
                sprintf(client_message,"%sOK",client_message); //adding ok if word is correct
                buf_insert_log(&buffer,client_message);
               	buf_remove_log(&buffer);
            }  else{
                sprintf(client_message,"%sMISPELLED",client_message); //adding mispelled if word is mispelled
                buf_insert_log(&buffer,client_message);
                buf_remove_log(&buffer);
            }            

      }
	
        write(sock , client_message , strlen(client_message)); 
        write(sock , prompt , strlen(prompt)); 
	   free(client_message);

    }
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    //Free the socket pointer
    free(socket_desc);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&full);
    pthread_cond_destroy(&empty);
    pthread_exit (NULL);

     
    return 0;
}


//initializes everything in sturcture
void buf_init(buf *sp, int job_len, int log_len){

    sp->job_len = job_len;
    sp->log_len = log_len;
    sp->job_front = 0;
    sp->log_front = 0;
    sp->job_rear = 0;
    sp->log_rear = 0;
    sp->job_count = 0;
    sp->log_count = 0;
    sp->count = 0;

}

void buf_insert_log(buf *sp, char *item){ //inserting an item into log queue

	pthread_mutex_lock(&mutex);
	while(sp->count == sp->log_len){
		pthread_cond_wait(&full,&mutex);//wait while it is full;
	}
	//critical section
   	sp->log_buf = (char**)malloc(5*sizeof(char));
	sp->log_buf[sp->log_count] = item;
	sp->log_count++;
    sp->count++;

	pthread_cond_signal(&empty);
	pthread_mutex_unlock(&mutex);
	sleep(1);

    }


void buf_insert_job(buf *sp, int item){
    sp->job_buf[sp->job_count] = item;
    sp->job_count++;
}
void buf_remove_log(buf *sp){
    
    pthread_mutex_lock(&mutex);
    while(sp->count == 0){
        pthread_cond_wait(&empty,&mutex); //wait while queue is full
    }
    fprintf(f, "%s\n", sp->log_buf[sp->log_front]); //adding word to log
    sp->log_front++;
    sp->count--;
    pthread_cond_signal(&full); //Signal a blocked thread who is in full condition
    pthread_mutex_unlock(&mutex);   //Realse lock
    /* Do some work so threads can alternate on mutex lock */
    sleep(1);
  }


void buf_remove_job(buf *sp){
    sp->job_front++;
}







