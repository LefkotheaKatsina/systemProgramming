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
#define h_addr h_addr_list[0] /* for backward compatibility */
/* /usr/lib/x86_64-linux-gnu */

void perror_exit (char *message);
int read_all(int,void*,int);
int write_all(int,void*,int);
void readFile(long,long,int,int);
int createPath(char*,int);
