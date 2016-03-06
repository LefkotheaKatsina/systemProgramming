#include "headerClient.h"

int main(int argc , char *argv[]){
    int server_port,sock,dirlen,fd,temp;
    long sizeInBytes,pagesize;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    char *server_ip;
    char *directory,*filepath;
// --------------------------------------------------------
    int rc,err,buflen = 256;
    struct hostent hbuf;
    struct hostent *result;
    char* buffer = malloc(buflen);
//---------------------------------------------------------
    if (argc != 7){
        printf("Please give right number of arguments\n");
        exit(1);
    }
    //dealing with flags and initialization of arguments
    if (!strcmp(argv[1],"-i")){
        server_ip = malloc(strlen(argv[2])+1);
        strcpy(server_ip,argv[2]);
    }
    if (!strcmp(argv[1],"-p"))
        server_port = atoi(argv[2]);
    if (!strcmp(argv[1],"-d")){
        directory = malloc(strlen(argv[2])+1);
        strcpy(directory,argv[2]);
    }
    if (!strcmp(argv[3],"-i")){
        server_ip = malloc(strlen(argv[4])+1);
        strcpy(server_ip,argv[4]);
    }
    if (!strcmp(argv[3],"-p"))
        server_port = atoi(argv[4]);
    if (!strcmp(argv[3],"-d")){
        directory = malloc(strlen(argv[4])+1);
        strcpy(directory,argv[4]);
    }
    if (!strcmp(argv[5],"-i")){
        server_ip = malloc(strlen(argv[6])+1);
        strcpy(server_ip,argv[6]);
    }
    if (!strcmp(argv[5],"-p"))
        server_port = atoi(argv[6]);
    if (!strcmp(argv[5],"-d")){
        directory = malloc(strlen(argv[6])+1);
        strcpy(directory,argv[6]);
    }
    //print message
    printf("Client's parameters are:\n Server_ip: %s\n port: %d\n directory: %s\n",server_ip,server_port,directory);
    fflush(stdout);
    /* Create socket */
    if ((sock = socket(PF_INET, SOCK_STREAM , 0)) < 0)
        perror_exit("socket");
    /* Find server address */
    while ((rc = gethostbyname_r(server_ip,&hbuf,buffer,buflen,&result,&err)) == ERANGE){
        buflen*=2;
        free(buffer);
        buffer = malloc(buflen);
        if (NULL == buffer){
            perror_exit("malloc");
        }
    }
    if (0 != rc || NULL == result) {
        perror("gethostbyname");
    }
    // Initialize server
    server.sin_family = AF_INET;
    memcpy (&server.sin_addr,result->h_addr,result->h_length);
    server.sin_port = htons(server_port);
    /* Initiate connection */
    printf("Connecting to %s on port %d ",server_ip,server_port);
    fflush(stdout);
    if (connect (sock,serverptr,sizeof(server)) < 0)
        perror_exit("connect");
    dirlen = strlen(directory)+1;
    if ( write_all(sock,&dirlen,sizeof(int)) != sizeof(int))
        perror_exit("write_all");
    if ( write_all(sock,directory,strlen(directory)+1) != strlen(directory)+1 ) //send the path of directory
        perror_exit ("write_all");
    while(1){
        //read the size of the filepath
        if (read_all(sock,&dirlen,sizeof(int)) != sizeof(int))
            perror_exit("read_all");
        if (dirlen != 0 ){ //if this is not the ack package
            filepath = malloc(dirlen);
            //read the path itself
            if (read_all(sock,filepath,dirlen) != dirlen)
                perror_exit ("read_all");
            //read the size of the file in bytes
            if (read_all(sock,&sizeInBytes,sizeof(long)) != sizeof(long))
                perror_exit("read_all");
            //read the pagesize
            if (read_all(sock,&pagesize,sizeof(long)) != sizeof(long))
                perror_exit("read_all");
            //receive the file itself
            temp = open(".",O_RDONLY); //hold current directory
            fd = createPath(filepath,dirlen);
            readFile(sizeInBytes,pagesize,sock,fd); //restore current directory
            printf("Received: %s\n",filepath);
            fchdir(temp);
        }
        else
            break;
    }
    free(buffer);
    free(server_ip);
    free(directory);
    return(0);
}

