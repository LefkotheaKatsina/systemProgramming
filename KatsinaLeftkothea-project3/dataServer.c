#include "header.h"

pthread_mutex_t mtx;
pthread_cond_t cond_empty;
pthread_cond_t cond_full;
Queue* workQueue;
pthread_t *cons;
int thread_pool_size;
int flag = 0;

int main(int argc, char *argv[]){
    int port,sock,newsock,err,queue_size,i;
    struct sockaddr_in server,client;
    socklen_t clientlen;
    pthread_t thread;
    information* info = malloc(sizeof(info));
    struct sockaddr *serverptr = (struct sockaddr *)&server;
    struct sockaddr *clientptr = (struct sockaddr *)&client;
    workQueue = createQueue();

    signal(SIGINT,signal_handler);

    if (argc != 7){
        printf("Please give right number of arguments!");
        exit(1);
    }
    // Initialize mutex
    pthread_mutex_init(&mtx,NULL);
    //Initialize cond variables
    pthread_cond_init(&cond_empty,NULL);
    pthread_cond_init(&cond_full,NULL);

    //dealing with flags and initialization of arguments
    if (!strcmp(argv[1],"-p"))
        port = atoi(argv[2]);
    if (!strcmp(argv[1],"-s"))
        thread_pool_size = atoi(argv[2]);
    if (!strcmp(argv[1],"-q"))
        queue_size = atoi(argv[2]);
    if (!strcmp(argv[3],"-p"))
        port = atoi(argv[4]);
    if (!strcmp(argv[3],"-s"))
        thread_pool_size = atoi(argv[4]);
    if (!strcmp(argv[3],"-q"))
        queue_size = atoi(argv[4]);
    if (!strcmp(argv[5],"-p"))
        port = atoi(argv[6]);
    if (!strcmp(argv[5],"-s"))
        thread_pool_size = atoi(argv[6]);
    if (!strcmp(argv[5],"-q"))
        queue_size = atoi(argv[6]);
    //print message
    cons = (pthread_t *)malloc(sizeof(pthread_t) * thread_pool_size);
    printf("Server's parameters are:\n port: %d\n thread_pool_size: %d\n queue_size: %d\n",port,thread_pool_size,queue_size);
    fflush(stdout);
    /* Create socket */
    if (( sock = socket(PF_INET,SOCK_STREAM,0) ) < 0)
        perror_exit ("socket");
    server.sin_family = AF_INET ;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    /* Bind socket to address */
    if ( bind (sock,serverptr,sizeof(server)) < 0)
        perror_exit ("bind");
    /* create worker threads*/
    for (i=0;i<thread_pool_size;i++){
        if ( err = pthread_create(cons+i,NULL,consumer,NULL) ){
            perror2("pthread_create",err);
            exit(1);
        }
    }
    //print message
    printf("Server was sucessfully initialized and ready to listen for connections...\n");
    fflush(stdout);
    /* Listen for connections*/
    if (listen(sock ,CONNECTIONS) < 0)
        perror_exit ("listen");
    printf("Listening for connections to port %d\n",port);
    fflush(stdout);

    while(1){
        /* accept connection */
        if ((newsock = accept(sock,clientptr,&clientlen)) < 0)
            perror_exit("accept");
        printf("Accepted connection from localhost\n");
        fflush(stdout);
        /* New thread */
        info->newsock = newsock;
        info->queue_size = queue_size;

        if ( err = pthread_create (&thread,NULL,connection_handler,(void*)info) ) {
            perror2 ("pthread_create",err );
            exit (1) ;
        }
    }
    pthread_exit(NULL);
}
