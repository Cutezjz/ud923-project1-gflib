#include <stdlib.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>
#include "gfserver.h"
#include "content.h"
#include "steque.h"

#define BUFFER_SIZE 4096

//mt implemetation
extern steque_t* queue;
extern pthread_mutex_t m;
extern pthread_cond_t c_cons;

typedef struct job{
	gfcontext_t *ctx;
	char *path;
	void *arg;
}job;

void job_create(job *this, gfcontext_t *ctx, char *path, void *arg){

	this->ctx = ctx;

	this->path = (char *)malloc(strlen(path) + 1);
	strcpy(this->path, path);

	this->arg = malloc(sizeof(arg) + 1);
	this->arg = arg;
}

void job_delete(job *this, gfcontext_t *ctx, char *path, void *arg){

	free((void *)this->arg);
	free((void *)this->path);
	free((void *)this->ctx);
	free((void *)this);
}

ssize_t handler_get(gfcontext_t *ctx, char *path, void* arg){
	//Lock mutex and push to queue
	printf("handler_get: LOCK\n");
	pthread_mutex_lock(&m);
		job *job_to_execute = (job *)malloc(sizeof(job));
		job_create(job_to_execute, ctx, path, arg);

		steque_push(queue, job_to_execute);
	pthread_mutex_unlock(&m);
	printf("handler_get: UNLOCK\n");
	pthread_cond_signal(&c_cons);

	return 0;

}

ssize_t handler_worker(void *thread_id){
	int id = *(int *)thread_id;
	while (1){
		printf("start handle worker %d\n", id);
		printf("handler_worker %d: LOCK\n", id);
		pthread_mutex_lock(&m);
			//If queue is empty wait
			while (steque_isempty(queue) != 0){
				printf("handle_worker %d: ENTERWAIT\n", id);
				pthread_cond_wait(&c_cons, &m);
				printf("handle_worker %d: EXITWAIT\n", id);
			}
			job *job_to_exec = (job *)steque_pop(queue);
		printf("handler_worker %d: UNLOCK\n", id);
		pthread_mutex_unlock(&m);
		int fildes;
		ssize_t file_len, bytes_transferred;
		ssize_t read_len, write_len;
		char buffer[BUFFER_SIZE];

		printf("handler_worker %d: path = %s\n", id, job_to_exec->path);

		if( 0 > (fildes = content_get(job_to_exec->path)))
			return gfs_sendheader(job_to_exec->ctx, GF_FILE_NOT_FOUND, 0);

		/* Calculating the file size */
		file_len = lseek(fildes, 0, SEEK_END);

		gfs_sendheader(job_to_exec->ctx, GF_OK, file_len);

		/* Sending the file contents chunk by chunk. */
		bytes_transferred = 0;
		while(bytes_transferred < file_len){
			read_len = pread(fildes, buffer, BUFFER_SIZE, bytes_transferred);
			if (read_len <= 0){
				fprintf(stderr, "handle_with_file read error, %zd, %zu, %zu", read_len, bytes_transferred, file_len );
				gfs_abort(job_to_exec->ctx);
				return -1;
			}
			write_len = gfs_send(job_to_exec->ctx, buffer, read_len);
			if (write_len != read_len){
				fprintf(stderr, "handle_with_file write error");
				gfs_abort(job_to_exec->ctx);
				return -1;
			}
			bytes_transferred += write_len;
			//printf("HANDLER: write_len = %zu\n", write_len);
			//printf("HANDLER: bytes_transferred = %zu\n", bytes_transferred);
		}

		printf("HANDLER %d: TOTAL bytes_transferred = %zu\n", id, bytes_transferred);
		//return bytes_transferred;
	}
}

