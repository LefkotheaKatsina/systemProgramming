#include "headerClient.h"

void perror_exit(char * message ){
    perror(message);
    exit(EXIT_FAILURE);
}
  int read_all(int fd , void* buff , int size){
    int sent,n;
    for( sent = 0;sent<size;sent += n ) {
        if (( n = read(fd,buff + sent,size - sent)) == -1)
            return -1; /* error */
    }
    return sent ;
}

int write_all(int fd,void* buff,int size){
    int sent,n;
    for( sent = 0;sent<size;sent += n) {
        if (( n = write(fd,buff + sent,size - sent)) == -1)
            return -1; /* error */
    }
    return sent ;
}

int createPath(char* filepath,int dirlen){
    char buffer[dirlen];
    int ret,fd;
    int i,j = 0;

    for (i=0;i<dirlen;i++){
        if ( i == 0 && filepath[i] == '/'){
            buffer[j] = '/';
            continue;
        }
        if (filepath[i] != '/'){
            buffer[j] = filepath[i];
            j++;
        }
        else{
            buffer[j] = '\0';
            mkdir(buffer,0755);
            if ((ret = chdir(buffer)) == -1)
                perror("chdir");
            j = 0;
        }
    }
    if ( (fd = open(buffer,O_CREAT|O_WRONLY|O_APPEND|O_TRUNC,0755) ) == -1)
        perror_exit("open");
    return fd;
}

void readFile(long sizeInBytes,long pagesize,int socket,int fd){
    long bytesLeft = sizeInBytes;
    char buffer[pagesize];

    while (bytesLeft > pagesize){
        if (read_all(socket,buffer,pagesize) != pagesize)
            perror_exit("read_all");
        if (write(fd,buffer,pagesize) < 0)
            perror_exit("write");
        bytesLeft-=pagesize;
    }
    if (bytesLeft != 0){
        if (read_all(socket,buffer,bytesLeft) != bytesLeft)
            perror_exit("read_all");
        if (write(fd,buffer,bytesLeft) < 0)
            perror_exit("write");
    }
    close(fd);
}
