#include "header.h"

extern pthread_mutex_t mtx;
extern pthread_cond_t cond_empty;
extern pthread_cond_t cond_full;
extern Queue* workQueue;
extern pthread_t *cons;
extern int thread_pool_size;
extern int flag;

Queue* createQueue()
{
    Queue* _queue = (Queue *)malloc( sizeof( Queue ) );
    _queue->head = _queue->last = NULL;
    _queue->_size = 0;
    return _queue;
}

QNode* createQNode()
{
    QNode* temp = (QNode *)malloc( sizeof( QNode ) ); // Allocate memory
    temp->prev = temp->next = NULL;
    return temp;
}

int IsQueueEmpty( Queue* _queue ){
    return (_queue->_size == 0);
}

void addNewNode(Queue* _queue,QNode* node){

    if ( IsQueueEmpty( _queue ) )// If queue is empty, change both head and last pointers
        _queue->last = _queue->head = node;
    else  // Else change the last
    {
        _queue->last->next = node;
        node->prev = _queue->last;
        _queue->last = node;
    }
    _queue->_size++;
}

void perror_exit (char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int read_all(int fd ,void* buff,int _size){
    int sent,n;
    for( sent = 0;sent<_size;sent += n ) {
        if ((n = read(fd,buff + sent,_size - sent)) == -1){
            return -1; /* error */
        }
    }
    return sent ;
}

int write_all(int fd,void* buff,int _size){
    int sent,n;
    for( sent = 0;sent<_size;sent += n) {
        if ((n = write(fd,buff + sent,_size - sent)) == -1){
            return -1; /* error */
        }
    }
    return sent ;
}

void place(Queue* workQueue,QNode* node,int queue_size){

    pthread_mutex_lock(&mtx);//protect data by locking the mutex
    while (workQueue->_size >= queue_size){
        //printf(">>WorkQueue is full\n");
        pthread_cond_wait(&cond_full,&mtx);
    }
    //printf("place to queue\n");
    addNewNode(workQueue,node);
    pthread_mutex_unlock(&mtx);
}

void *consumer(void * ptr){
    QNode* node;
    int dirlen,err;
    long pagesize = sysconf(_SC_PAGESIZE);
//--------------------------------------------
    while(1){
        pthread_mutex_lock(&mtx);//protect data by locking the mutex
        while(workQueue->_size <= 0){
            //printf(">>WorkQueue is empty\n");
            pthread_cond_wait(&cond_empty,&mtx);
            if (flag == 1){
                pthread_mutex_unlock(&mtx);
                printf("I am a consumer and i am ready to go!\n");
                pthread_exit(NULL);
            }
        }
        node = workQueue->head;
        workQueue->head = node->next;
        if(workQueue->_size == 1)
            workQueue->last = node->next;
        workQueue->_size--;

        pthread_mutex_lock(node->clientMutex);//lock the client mute
        pthread_mutex_unlock(&mtx);//unlock the mutex
        pthread_cond_broadcast(&cond_full);
        //1) send the size of the path or ack
        if (node->buf == NULL) {//if this is the last package to a certain client
            dirlen = 0; //send ack
            printf("[Thread: %ld]: Received task:<ACK,%d>\n",pthread_self(),node->client_sock);
            fflush(stdout);
        }
        else
            dirlen = strlen(node->buf)+1;
        //send the size of the path
        if (write_all(node->client_sock,&dirlen,sizeof(int)) != sizeof(int))
            perror_exit("write");
        if (dirlen != 0){ //if this is not the last pack to a certain client
            printf("[Thread: %ld]: Received task:<%s,%d>\n",pthread_self(),node->buf,node->client_sock);
            fflush(stdout);
            // 2)send the path itself
            if (write_all(node->client_sock,node->buf,dirlen) != dirlen)
                perror_exit ("write");
            // 3)send the size of the file
            if (write_all(node->client_sock,&node->sizeInBytes,sizeof(long)) != sizeof(long))
                perror_exit("write");
            // 4)send the pagesize
            if (write_all(node->client_sock,&pagesize,sizeof(long)) != sizeof(long))
                perror_exit("write");
            // 5)send the file itself
            printf("[Thread: %ld]: About to read file %s\n",pthread_self(),node->buf);
            fflush(stdout);
            writeFile(node,pagesize);
            pthread_mutex_unlock(node->clientMutex);
            free(node->buf);
        }
        else{//if this is the last package to a certain client
            close(node->client_sock);
            pthread_mutex_unlock(node->clientMutex);
            if ( err = pthread_mutex_destroy(node->clientMutex)){
                perror2("pthread_mutex_destroy",err);
                exit(1);
            }
            free(node->clientMutex);
        }
        free(node);
    }
}

void writeFile(QNode* node,long pagesize){
    long bytesLeft = node->sizeInBytes;
    char buffer[pagesize];
    int fd;
    if ( (fd = open(node->buf,O_RDONLY) ) == -1)
        perror_exit("open");
    while (bytesLeft > pagesize){
        if (read(fd,buffer,pagesize) < 0)
            perror_exit("read");
        if (write_all(node->client_sock,buffer,pagesize) != pagesize)
            perror_exit("write_all");
        bytesLeft-=pagesize;
    }
    if (bytesLeft != 0){
        if (read(fd,buffer,bytesLeft) < 0)
            perror_exit("read");
        if (write_all(node->client_sock,buffer,bytesLeft) != bytesLeft)
            perror_exit("write_all");
    }
    close(fd);
}

void *connection_handler(void *arg){
    int err,length;
    char* directory;
    QNode* node;
    int newsock = ((information*)arg)->newsock;
    int queue_size = ((information*)arg)->queue_size;
    pthread_mutex_t *clientMutex = malloc(sizeof(pthread_mutex_t));
//---------------------------------------------
    if ( err = pthread_detach(pthread_self())){
        perror2("pthread_detach",err);
        exit(1);
    }
    pthread_mutex_init(clientMutex,NULL ) ; /* Initialize client mutex */

    if (read_all(newsock,&length,sizeof(int)) != sizeof(int) )
        perror_exit("read");
    directory = malloc(length);
    if (read_all(newsock,directory,length) != length)
        perror_exit("read");
    //print directory
    //printf("directory is: %s\n",directory);
    printf("[Thread: %ld]: About to scan directory Server\n",pthread_self());
    fflush(stdout);
    findFiles(directory,newsock,queue_size,clientMutex);
    //create ack
    node = createQNode();
    node->client_sock = newsock;
    node->buf = NULL;
    node->clientMutex = clientMutex;
    place(workQueue,node,queue_size);
    pthread_cond_broadcast(&cond_empty);
    pthread_exit(NULL);
}
//for the functions read_all and write_all i used an internet resource

void findFiles(char* directory,int newsock,int queue_size,pthread_mutex_t *clientMutex){
    int error;
    DIR *dirp;
    struct dirent *ent;
    struct dirent *dp = malloc(sizeof(struct dirent));
    struct stat statbuf;
    char* path;
    QNode* node;

    if ((dirp = opendir(directory)) == NULL){
        perror_exit("open directory");
        return ;
    }
    while ( ( (error = readdir_r(dirp,dp,&ent) )== 0 ) && ent ){
        if ( !strcmp(ent->d_name,".") || !strcmp(ent->d_name,".."))
            continue;
        path = malloc(strlen(directory)+strlen(ent->d_name)+2);
        sprintf(path,"%s/%s",directory,ent->d_name);
        if (stat(path,&statbuf) == -1)
            perror_exit("stat");
        //check if it is file or directory
        if ((statbuf.st_mode & S_IFMT) != S_IFDIR ){ //we found a file
            //create and initialize node
            printf("[Thread: %ld]: Adding file %s to the queue...\n",pthread_self(),path);
            fflush(stdout);
            node = createQNode();
            node->client_sock = newsock;
            node->buf = malloc(strlen(path)+1);
            node->clientMutex = clientMutex;
            strcpy(node->buf,path);//buf holds the path of the file
            node->sizeInBytes = statbuf.st_size;//sizeInBytes holds the size of the file in bytes
            place(workQueue,node,queue_size);
            pthread_cond_broadcast(&cond_empty);
        }
        else //we found a directory
            findFiles(path,newsock,queue_size,clientMutex);
    }
    if (error)
        perror_exit("readdir_r");
    else
        closedir(dirp);
}

void signal_handler(int signum){
    int err,i = 0;
    printf("I am the signal_handler of the Server\n");
    flag = 1;
    pthread_cond_broadcast(&cond_empty);
    for(i=0;i<thread_pool_size;i++)
        /* Wait for all consumers termination */
        if(err = pthread_join(*(cons + i),NULL)){
            perror2("pthread_join",err);
                exit (1) ;
        }
    free(cons);
    if ( err = pthread_mutex_destroy(&mtx)){
        perror2("pthread_mutex _destroy",err);
        exit(1);
    }
    if ( err = pthread_cond_destroy(&cond_empty)){
        perror2("pthread_cond_ destroy",err);
        exit(1);
    }
    if ( err = pthread_cond_destroy(&cond_full)){
        perror2("pthread_cond_ destroy",err);
        exit(1);
    }
    free(workQueue);
    printf("Server says:Adios muchachos!\n");
    exit(EXIT_SUCCESS);
}


