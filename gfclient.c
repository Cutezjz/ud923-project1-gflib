#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h> 
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include "gfclient.h"

//Stewart's own functions
//void open_file_and_write (int sock, char * filename);
int create_socket(gfcrequest_t *gfr, struct sockaddr_in * serv_addr);
int gfc_intstatus(char * status);

typedef struct gfcrequest_t{
	char *server;
	char *path;
	unsigned short port;
	gfstatus_t status;
	void (*headerfunc)(void*, size_t, void *);
	void *headerarg;
	void (*writefunc)(void*, size_t, void *);
	void *writearg;
	size_t bytesreceived;
	size_t file_size;
}gfcrequest_t;

gfcrequest_t *gfc_create(){
	/*
	 * This function must be the first one called as part of
	 * request.  It returns a gfcrequest_t handle which should be
	 * passed into all subsequent library calls pertaining to
	 * this requeest.
	 */
	gfcrequest_t *gfr = (gfcrequest_t *)malloc(sizeof(gfcrequest_t));
	gfr->headerfunc = NULL;
	gfr->headerarg = NULL;
	gfr->writefunc = NULL;
	gfr->writearg = NULL;
	gfr->bytesreceived = 0;
	gfr->file_size = 0;

	return gfr;
}

void gfc_set_server(gfcrequest_t *gfr, char* server){

	/*
	 * Sets the server to which the request will be sent.
	 */

	gfr->server = (char *)malloc(strlen(server) + 1);
	if (gfr->server == NULL){
		perror("ERROR SETTING SERVER\n");

	}
 	strcpy(gfr->server, server);
}

void gfc_set_path(gfcrequest_t *gfr, char* path){

	/*
	 * Sets the path of the file that will be requested.
	 */
	gfr->path = (char *)malloc(strlen(path) + 1);
	if (gfr->path == NULL){
		perror("ERROR SETTING PATH\n");

	}
	//localPath(path, gfr->path);
	strcpy(gfr->path, path);
}

void gfc_set_port(gfcrequest_t *gfr, unsigned short port){
	/*
	 * Sets the port over which the request will be made.
	 */
	gfr->port = port;
}

void gfc_set_headerfunc(gfcrequest_t *gfr, void (*headerfunc)(void*, size_t, void *)){
	/*
	 * Sets the callback for received header.  The registered callback
	 * will receive a pointer the header of the response, the length
	 * of the header response as it's second argument (don't assume that
	 * this is null-terminated), and the pointer registered with
	 * gfc_set_headerarg (or NULL if not specified) as the third argument.
	 *
	 * You may assume that the callback will only be called once and will
	 * contain the full header.
	 */
	 gfr->headerfunc = headerfunc;

}

void gfc_set_headerarg(gfcrequest_t *gfr, void *headerarg){
	/*
	 * Sets the third argument for all calls to the registered header callback.
	 */
	gfr->headerarg = malloc(sizeof headerarg);
	if (gfr->headerarg == NULL){
		perror("ERROR SETTING headerarg\n");
	}
	gfr->headerarg = headerarg;
}

void gfc_set_writefunc(gfcrequest_t *gfr, void (*writefunc)(void*, size_t, void *)){
	/*
	 * Sets the callback for received chunks of the body.  The registered
	 * callback will receive a pointer the chunk, the length of the chunk
	 * as it's second argument (don't assume that this is null-terminated),
	 * and the pointer registered with gfc_set_writearg (or NULL if not
	 * specified) as the third argument.
	 *
	 * The callback may be called multiple times in a single request.  The
	 * gfclient library does not store the entire contents of the requested file
	 * but rather calls this callback each time that it receives a chunk of data
	 * from the server.
	 */
	gfr->writefunc = writefunc;
}

void gfc_set_writearg(gfcrequest_t *gfr, void *writearg){
	/*
	 * Sets the third argument for all calls to the registered header callback.
	 */
	gfr->writearg = malloc(sizeof writearg);
	if (gfr->writearg == NULL){
		perror("ERROR SETTING writearg\n");
	}
	gfr->writearg = writearg;
}
/*
int create_request_str(char * path){
	const char* line_ending = "\r\n\r\n";
	const char* line_beg = "GETFILE GET";
	char *request_str = (char *)malloc(strlen(line_beg)+  strlen(path) +  strlen(line_ending) + 3);

	sprintf(request_str, "%s %s %s", line_beg, path, line_ending);
	return request_str;
}
*/

int gfc_perform(gfcrequest_t *gfr){

	/*
	 * Performs the transfer as described in the options.  Returns a value of 0
	 * if the communication is successful, including the case where the server
	 * returns a response with a FILE_NOT_FOUND or ERROR response.  If the
	 * communication is not successful (e.g. the connection is closed before
	 * transfer is complete or an invalid header is returned), then a negative
	 * integer will be returned.
	 */
	int sockfd = 0; //socket file descriptor
	struct sockaddr_in serv_addr;
	const char* line_ending = "\r\n\r\n";
	const char* line_beg = "GETFILE GET";
	char header[1000] ="";
	char *p_header = header;
	int bytes_read;
	int total_bytes_read = 0;
	int header_size = 200;
	int ret_scanf;
	size_t file_size = 0;
	char str_status[15];
	//Create request_str
	char *request_str = (char *)malloc(strlen(line_beg) + strlen(gfr->path) +  strlen(line_ending) + 3);
	sprintf(request_str, "%s %s%s", line_beg, gfr->path, line_ending);

	//test prints
	printf("This is the server: %s\n", gfr->server);
	printf("This is the port: %d\n", gfr->port);
	printf("This is the path: %s\n", gfr->path);
	printf("This is the request str: %s\n", request_str);

	sockfd = create_socket(gfr, &serv_addr);
	if (sockfd < 0){
		perror("hello\n");
		return EXIT_FAILURE;
	}

	printf("CLIENT: This is the request str sent: %s\n", request_str);
    send(sockfd, request_str, strlen(request_str), 0);


    //get header
    while (1){
    	bzero(p_header, header_size);
    	bytes_read = recv(sockfd, (void *)p_header, header_size,0);
    	p_header += bytes_read;
    	header_size-=bytes_read;
    	total_bytes_read+=bytes_read;
    	printf("client: bytes read %d\n", bytes_read);
    	if (bytes_read == 0){
    		break;
    	}
    	else if (bytes_read < 0){
    		perror("ERROR reading socket");
    	}
    }
    if (gfr->headerfunc == NULL)
    	puts("CLIENT: Headerfunction not set");
    else
    	gfr->headerfunc((void*)header, total_bytes_read, gfr->headerarg);
    //get file size from header
	ret_scanf = sscanf(header, "GETFILE %s %zu\r\n\r\n", str_status, &file_size);
	printf("str status = %s, filzesize = %zu\n", str_status, file_size);
	if (ret_scanf == EOF){
		perror("PATH WAS PARSED but fpath not found\n");
		gfr->status = GF_FILE_NOT_FOUND;
	}
	else{
		gfr->status = gfc_intstatus(str_status);
		gfr->file_size = file_size;
	}
    //get data
	total_bytes_read = 0;
	char * file_data = (char *)calloc(gfr->file_size, sizeof(char));
	char *p_file_data = file_data;
	size_t data_remaining = gfr->file_size;
    //while (total_bytes_read < gfr->file_size){
	while (1){
    	bytes_read = recv(sockfd, (void *)p_file_data, data_remaining,0);
		//bytes_read = recv(sockfd, (void *)file_data, data_remaining,0);
    	if (bytes_read == 0){
    		break;
    	}
    	else if (bytes_read < 0){
    		perror("ERROR reading socket");
    	}
    	p_file_data += bytes_read;
    	data_remaining-=bytes_read;
    	total_bytes_read+=bytes_read;
    	printf("bytes_read = %d\n", bytes_read);
    	printf("total bytes_read = %d\n", total_bytes_read);
    	if (gfr->writefunc == NULL)
    		puts("CLIENT: WRITEFUNC not set");
    	else
    		gfr->writefunc((void *)p_file_data, bytes_read, gfr->writearg);
    }

	puts("COMPLETE CLIENT\n");
	free(request_str);
	return 0;
}

int create_socket(gfcrequest_t *gfr, struct sockaddr_in * serv_addr){
	/*
	 This function uses gfr struct to create socket.
	 If no exceptions are raised the socket file descriptor is returned.
	 */
	int sockfd = 0;
	struct hostent *server;
	server = gethostbyname(gfr->server);
	if(server == NULL){
		perror("Error: No such host");
		return EXIT_FAILURE;
	}

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Error: Could not create socket");
		return EXIT_FAILURE;
	}
	bzero((char *) serv_addr, sizeof(serv_addr));
	serv_addr->sin_family = AF_INET;
	bcopy((char *)server->h_addr,
			(char *)&serv_addr->sin_addr.s_addr,
			server->h_length);
	serv_addr->sin_port = htons(gfr->port);

	if(connect(sockfd, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) == -1){
		perror("Could not establish connection");
		return EXIT_FAILURE;
	}
	return sockfd;
}

void write_to_socket(int sock, char * buffer){
	int bytes_read = strlen(buffer);
	int bytes_written;
	void *p = buffer;
	while (bytes_read > 0){
		bytes_written = send(sock, p, bytes_read,0);
		printf("client: bytes written %d\n", bytes_written);
		printf("written: '%s'", (char *)p);
		if (bytes_written <= 0){
			perror("pERROR writing to socket");
		}
		bytes_read -= bytes_written;
		p+=bytes_written;
	}
}
void read_and_print (int sock)
{

    int bytes_read;
    char buffer[4096];

    while (1){

    	bytes_read = recv(sock, (void *)buffer, sizeof(buffer),0);
    	printf("client: bytes read %d\n", bytes_read);
    	if (bytes_read == 0){
    		break;
    	}
    	else if (bytes_read < 0){
    		perror("ERROR reading socket");
    	}

    	printf("CLIENT: this was read from socket '%s'\n", (char *)buffer);
    }
    puts("complete client\n");

}
gfstatus_t gfc_get_status(gfcrequest_t *gfr){
	/*
	 * Returns the status of the response.
	 */

	return gfr->status;
}

/*
 * Returns the status of the response.
 */
char* gfc_strstatus(gfstatus_t status){
	/*
	 * Returns the string associated with the input status
	 */
	if (status == GF_OK)
	{
		return "OK";
	}
	else if (status == GF_FILE_NOT_FOUND)
	{
		return "FILE_NOT_FOUND";
	}
	else
	{
		return "ERROR";
	}
}

int gfc_intstatus(char * status){
	/*
	 * Returns the string associated with the input status
	 */
	const char *ok = "OK";
	const char *fnf = "FILE_NOT_FOUND";
	const char *err = "ERROR";
	if (strncmp(status, ok, strlen(ok)) == 0)
		return GF_OK;
	else if (strncmp(status, fnf, strlen(fnf)) == 0)
		return GF_FILE_NOT_FOUND;
	else if (strncmp(status, err, strlen(err)) == 0)
		return GF_ERROR;
	else{
		printf("gfc_intstatus: status given not of appropiate form '%s'", status);
		return -1;
	}
}

size_t gfc_get_filelen(gfcrequest_t *gfr){
	/*
	 * Returns the length of the file as indicated by the response header.
	 * Value is not specified if the response status is not OK.
	 */
	if (gfr != NULL && gfc_get_status(gfr) == GF_OK)
	{
		return sizeof(gfr->path);
	}
	else{
		return -1;
	}
}

size_t gfc_get_bytesreceived(gfcrequest_t *gfr){

	/*
	 * Returns actual number of bytes received before the connection is closed.
	 * This may be distinct from the result of gfc_get_filelen when the response
	 * status is OK but the connection is reset before the transfer is completed.
	 */
	return gfr->bytesreceived;
}

void gfc_cleanup(gfcrequest_t *gfr){
	/*
	 * Frees memory associated with the request.
	 */
	free(gfr->server);
	free(gfr->path);
	free(gfr);
}

void gfc_global_init(){
	/*
	 * Sets up any global data structures needed for the library.
	 * Warning: this function may not be thread-safe.
	 */
	puts("3");
}


void gfc_global_cleanup(){
	/*
	 * Cleans up any global data structures needed for the library.
	 * Warning: this function may not be thread-safe.
	 */
	puts("3");
}
