#include <unistd.h>
#include "gfserver.h"

/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */
typedef struct gfserver_t{
	//char * server;
	//char * path;
	unsigned short port;
	int max_npending;
	//int status;
	ssize_t (*handlerfunc)(gfcontext_t *, char *, void*);
	void *handlerarg;
	//void (*writefunc)(void*, size_t, void *);
	//void *writearg;
	//size_t bytesreceived;
}gfserver_t;

ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len){

	/*
	 * Sends to the client the Getfile header containing the appropriate
	 * status and file length for the given inputs.  This function should
	 * only be called from within a callback registered gfserver_set_handler.
	 */
}

ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len){
	/*
	 * Sends size bytes starting at the pointer data to the client
	 * This function should only be called from within a callback registered
	 * with gfserver_set_handler.  It returns once the data has been
	 * sent.
	 */
}

void gfs_abort(gfcontext_t *ctx){
	/*
	 * Aborts the connection to the client associated with the input
	 * gfcontext_t.
	 */
}

gfserver_t* gfserver_create(){
	/*
	 * This function must be the first one called as part of
	 * setting up a server.  It returns a gfserver_t handle which should be
	 * passed into all subsequent library calls of the form gfserver_*.  It
	 * is not needed for the gfs_* call which are intended to be called from
	 * the handler callback.
	 */
	//return *gfserver_t;
}

void gfserver_set_port(gfserver_t *gfs, unsigned short port){

	/*
	 * Sets the port at which the server will listen for connections.
	 */
	gfs->port = port;
}
void gfserver_set_maxpending(gfserver_t *gfs, int max_npending){
	/*
	 * Sets the maximum number of pending connections which the server
	 * will tolerate before rejecting connection requests.
	 */
	gfs->max_npending = max_npending;
}

void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, char *, void*)){
	/*
	 * Sets the handler callback, a function that will be called for each each
	 * request.  As arguments, the receives
	 * - a gfcontext_t handle which it must pass into the gfs_* functions that
	 * 	 it calls as it handles the response.
	 * - the requested path
	 * - the pointer specified in the gfserver_set_handlerarg option.
	 * The handler should only return a negative value to signal an error.
	 */
	gfs->handlerfunc = handler;
}

void gfserver_set_handlerarg(gfserver_t *gfs, void* arg){
	/*
	 * Sets the third argument for calls to the handler callback.
	 */
	gfs->handlerarg = arg;
}

void gfserver_serve(gfserver_t *gfs){
	/*
	 * Starts the server.  Does not return.
	 */
	  //bzero((char *) &serv_addr, sizeof(serv_addr));
	//  serv_addr.sin_family = AF_INET;
	//  serv_addr.sin_addr.s_addr = INADDR_ANY;
	//  serv_addr.sin_port = htons(portno);
    //
	//  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1){
	//// error("Error on setting options");
	//  }
	//  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
	//           error("ERROR on binding");
	//  }
	//  if (listen(sockfd,5) == -1){
	//// error("ERROR on listen");
	//  }
	//  clilen = sizeof(cli_addr);
}

