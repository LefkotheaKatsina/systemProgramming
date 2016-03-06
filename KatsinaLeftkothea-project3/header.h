#define _POSIX_SOURCE
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#define perror2(s,e) fprintf (stderr,"%s: %s\n",s,strerror(e))
#define h_addr h_addr_list[0] /* for backward compatibility */
#define S_IFMT  0170000 /* type of file */
#define S_IFDIR 0040000 /* directory */
/* /usr/lib/x86_64-linux-gnu */

#define CONNECTIONS 15

typedef struct QNode
{
    char* buf;
    long sizeInBytes;
    int client_sock;
    pthread_mutex_t *clientMutex;
    struct QNode *prev, *next; //pointer to next QNode
} QNode;

typedef struct Queue
{
    QNode *head,*last;
    int _size;
} Queue;

typedef struct Information
{
    int newsock;
    int queue_size;
}information;

Queue* createQueue();
QNode* createQNode();
int IsQueueEmpty( Queue*);
void deleteNode(Queue*,QNode*);
void addNewNode(Queue*,QNode*);
void perror_exit (char *);
void place(Queue*,QNode* ,int);
void *consumer(void *);
void *connection_handler(void *);

void signal_handler(int);
void consumer_handler(int);

void perror_exit(char *);
void *connection_handler(void *);
Queue* createQueue();
int read_all(int,void*,int);
int write_all(int,void*,int);
void findFiles(char*,int,int,pthread_mutex_t*);
void writeFile(QNode*,long);
