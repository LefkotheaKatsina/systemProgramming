all : dataServer remoteClient

dataServer : dataServer.c functions.c
	gcc -o dataServer dataServer.c functions.c -lpthread

remoteClient : remoteClient.c functionsClient.c
	gcc -o remoteClient remoteClient.c functionsClient.c -lpthread
