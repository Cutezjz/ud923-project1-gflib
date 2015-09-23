#include <unistd.h>
#include "gfserver.h"

//sb additional includes
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

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

typedef struct gfcontext_t{
	int sockfd;
	gfstatus_t status;
}gfcontext_t;

//SB FUNCTIONS
int get_path(gfcontext_t *gfc, char *fpath);

ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len){

	/*
	 * Sends to the client the Getfile header containing the appropriate
	 * status and file length for the given inputs.  This function should
	 * only be called from within a callback registered gfserver_set_handler.
	 */
	//send(newsockfd, (void*)temp_str, sizeof(temp_str), 0);3



	char *scheme = "GETFILE";
	char int_str[15];
	char str_status[15];

	//Create string from integer of file length
	sprintf(int_str, "%zu", file_len);
	//Get a string status from macro definitions
	if (status == GF_OK){
		strcpy(str_status, "OK");
	}
	else if (status == GF_FILE_NOT_FOUND){
		strcpy(str_status, "FILE_NOT_FOUND");
	}
	else{
		strcpy(str_status, "ERROR");
	}
	char *header = (char *)malloc(strlen(scheme) + strlen(str_status) + strlen(int_str) + 3);
	sprintf(header, "%s %s %s", scheme, str_status, int_str);
	return send(ctx->sockfd, (void*)header, sizeof(header), 0);

}

ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len){
	/*
	 * Sends size bytes starting at the pointer data to the client
	 * This function should only be called from within a callback registered
	 * with gfserver_set_handler.  It returns once the data has been
	 * sent.
	 */

	return send(ctx->sockfd, (void*)data, len, 0);
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
	gfserver_t *gfs = (gfserver_t *)malloc(sizeof(gfserver_t));
	return gfs;
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
	gfs->handlerarg = malloc(sizeof(arg) + 1);
	gfs->handlerarg = arg;
}

void gfserver_serve(gfserver_t *gfs){
	/*
	 * Starts the server.  Does not return.
	 */
	  //int portno = 8888; /* port to listen on */
	  int sockfd, newsockfd;
	  char client_req_path[1000];
	  socklen_t clilen;
	  struct sockaddr_in serv_addr, cli_addr;
	  char fpath[256];

	  sockfd = socket(AF_INET, SOCK_STREAM, 0);
	  if (sockfd < 0){
		  perror("ERROR opening socket");
	  }
	  bzero((char *) &serv_addr, sizeof(serv_addr));
	  serv_addr.sin_family = AF_INET;
	  serv_addr.sin_addr.s_addr = INADDR_ANY;
	  serv_addr.sin_port = htons(gfs->port);

	  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1){
		  perror("Error on setting options");
	   }
	  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
	          perror("ERROR on binding");
	  }
	  if (listen(sockfd,5) == -1){
		  perror("ERROR on listen");
	  }
	  clilen = sizeof(cli_addr);

	  while (1){
		  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		  if (newsockfd < 0){
			  perror("ERROR on accept");
		  }
		  gfcontext_t *gfc = (gfcontext_t *)malloc(sizeof(gfcontext_t));
		  gfc->sockfd = newsockfd;

		  bzero(client_req_path, 1000);
		  recv(gfc->sockfd , (void*)client_req_path, sizeof(client_req_path), 0);
		  printf("SERVER:This is the mess from the client %s\n", client_req_path);
		  //(gfcontext_t *, char *, void*)
		  //gfs->handlerfunc(gfc, client_req_path, gfs->handlerarg);
		  get_path(gfc, &fpath);
	  }
}

int get_path(gfcontext_t *gfc, char *fpath){
	int bytes_read = 0;
	int rem_arr = 1000;
	char client_req_path[1000] = "";
	char *p = client_req_path;
	char *beg_path = "GETFILE GET ";
	char *end_path = "\r\n\r\n";
	int index = 0;
	while (1){
		bytes_read = recv(gfc->sockfd , (void*)p, rem_arr, 0);
		if (bytes_read < 0){
			perror("Error reading socket\n");
		}
		else if (bytes_read == 0){
			break;
		}
		else{
			p+=bytes_read;
			rem_arr-=bytes_read;
		}
	}

	//Check that string begins with GETFILE GET
	int beg_compare_length = strlen(beg_path);

	if (strncmp(beg_path, client_req_path, beg_compare_length) !=0){
		gfc->status = GF_FILE_NOT_FOUND;
		return -1;
	}


	p = &client_req_path[beg_compare_length];
	char *p_fpath = fpath;
	int break_bool = 0;
	while (*p != 0){
		if (strncmp(p, end_path, strlen(end_path)) == 0){
			break_bool = 1;
			break;
		}
		*p_fpath = *p;
		p_fpath++;
		p++;
	}
	//If the loop wasn't broken out of by strncmp than the syntax
	//wasnt correct in sent path
	if (break_bool == 0){
		gfc->status = GF_FILE_NOT_FOUND;
		return -1;
	}
	if( access(fpath, F_OK) != -1){
		gfc->status = GF_OK;
		return 0;
	}
	else{
		gfc->status = GF_FILE_NOT_FOUND;
		return -1;
	}


}

