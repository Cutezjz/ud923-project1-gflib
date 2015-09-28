#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <pthread.h>
#include "steque.h"
#include "workload.h"
#include "gfclient.h"

steque_t* queue;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_cons = PTHREAD_COND_INITIALIZER;
int jobs_completed = 0;
int tot_jobs;

typedef struct job{
	gfcrequest_t *gfr;
	FILE *file;
	char local_path[512];
}job;

void client_worker(void *thread_id);

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  webclient [options]\n"                                                     \
"options:\n"                                                                  \
"  -s [server_addr]    Server address (Default: 0.0.0.0)\n"                   \
"  -p [server_port]    Server port (Default: 8888)\n"                         \
"  -w [workload_path]  Path to workload file (Default: workload.txt)\n"       \
"  -t [nthreads]       Number of threads (Default 1)\n"                       \
"  -n [num_requests]   Requests download per thread (Default: 1)\n"           \
"  -h                  Show this help message\n"                              \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"server",        required_argument,      NULL,           's'},
  {"port",          required_argument,      NULL,           'p'},
  {"workload-path", required_argument,      NULL,           'w'},
  {"nthreads",      required_argument,      NULL,           't'},
  {"nrequests",     required_argument,      NULL,           'n'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};



static void Usage() {
	fprintf(stdout, "%s", USAGE);
}

static void localPath(char *req_path, char *local_path){
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE* openFile(char *path){
  char *cur, *prev;
  FILE *ans;

  /* Make the directory if it isn't there */
  prev = path;
  while(NULL != (cur = strchr(prev+1, '/'))){
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU)){
      if (errno != EEXIST){
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if( NULL == (ans = fopen(&path[0], "w"))){
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}

/* Callbacks ========================================================= */
static void writecb(void* data, size_t data_len, void *arg){
  FILE *file = (FILE*) arg;

  fwrite(data, 1, data_len, file);
}

/* Main ========================================================= */
int main(int argc, char **argv) {
/* COMMAND LINE OPTIONS ============================================= */
  char *server = "localhost";
  unsigned short port = 8888;
  char *workload_path = "workload.txt";

  int i;
  int option_char = 0;
  int nrequests = 1;
  int nthreads = 2;
  int returncode;
  //gfcrequest_t *gfr;
  char *req_path;

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "s:p:w:n:t:h", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 's': // server
        server = optarg;
        break;
      case 'p': // port
        port = atoi(optarg);
        break;
      case 'w': // workload-path
        workload_path = optarg;
        break;
      case 'n': // nrequests
        nrequests = atoi(optarg);
        break;
      case 't': // nthreads
        nthreads = atoi(optarg);
        break;
      case 'h': // help
        Usage();
        exit(0);
        break;                      
      default:
        Usage();
        exit(1);
    }
  }

  if( EXIT_SUCCESS != workload_init(workload_path)){
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  gfc_global_init();

  queue = malloc(sizeof(steque_t));
  steque_init(queue);
  int *thread_id = malloc(sizeof(int) * nthreads);

  tot_jobs = nrequests * nthreads;
  job *jobs = malloc(sizeof(job) * tot_jobs);
  //create nthreads threads
  pthread_t *thread_list = (pthread_t *)malloc(nthreads * sizeof(pthread_t));

  for (int ii =0; ii < nthreads; ii++){
	  thread_id[ii] = ii;
	  //pthread_create(thread_list + ii, NULL, (void *)&client_worker, (void *)&thread_id[ii]);
	  pthread_create(&thread_list[ii], NULL, (void *)&client_worker, (void *)&thread_id[ii]);
  }
  /*Making the requests...*/
  for(i = 0; i < tot_jobs; i++){
    req_path = workload_get_path();

    if(strlen(req_path) > 256){
      fprintf(stderr, "Request path exceeded maximum of 256 characters\n.");
      exit(EXIT_FAILURE);
    }
    job *job_to_execute = (job *)malloc(sizeof(job));
    //char local_path[512];
    localPath(req_path, job_to_execute->local_path);

    job_to_execute->file = openFile(job_to_execute->local_path);

    job_to_execute->gfr = gfc_create();
    gfc_set_server(job_to_execute->gfr, server);
    gfc_set_path(job_to_execute->gfr, req_path);
    gfc_set_port(job_to_execute->gfr, port);
    gfc_set_writefunc(job_to_execute->gfr, writecb);
    gfc_set_writearg(job_to_execute->gfr, job_to_execute->file);

    fprintf(stdout, "Requesting %s%s\n", server, req_path);

    //returncode = gfc_perform(job_to_execute->gfr);
	printf("handler_get: LOCK\n");
	pthread_mutex_lock(&m);
		steque_push(queue, job_to_execute);
	pthread_mutex_unlock(&m);

	printf("handler_get: UNLOCK\n");
	pthread_cond_signal(&c_cons);

  }
  for (int ii =0; ii < nthreads; ii++){
	  thread_id[ii] = ii;
	  pthread_join(thread_list[ii], NULL);
  }
  gfc_global_cleanup();

  return 0;
}

void client_worker(void *thread_id){
	int id = *(int *)thread_id;
	int iii = 0;
	while (1){
		printf("start  client_worker %d - loop %d\n", id, iii);
		printf("client_worker %d - loop %d: LOCK\n", id, iii);
		pthread_mutex_lock(&m);
			printf("jobs_completed = %d\n", iii);
			//wait while steque_isempty
			if (jobs_completed < tot_jobs){
				while (steque_isempty(queue) != 0){
					printf("client_worker %d: ENTERWAIT\n", id);
					pthread_cond_wait(&c_cons, &m);
					printf("client_worker %d: EXITWAIT\n", id);
				}
			}
			else{
				pthread_mutex_unlock(&m);
				break;
			}
			job *job_to_exec = (job *)steque_pop(queue);
			jobs_completed++;
			printf("local_path = %s\n", job_to_exec->local_path);
		pthread_mutex_unlock(&m);
		printf("client_worker %d: before gfc_perform\n", id);
		int returncode = gfc_perform(job_to_exec->gfr);
		printf("client_worker %d: after gfc_perform\n", id);
		if ( returncode < 0){
		  fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
		  fclose(job_to_exec->file);
		  if ( 0 > unlink(job_to_exec->local_path))
			fprintf(stderr, "unlink failed on %s\n", job_to_exec->local_path);
		}
		else {
			fclose(job_to_exec->file);
		}

		if ( gfc_get_status(job_to_exec->gfr) != GF_OK){
		  if ( 0 > unlink(job_to_exec->local_path))
			fprintf(stderr, "unlink failed on %s\n", job_to_exec->local_path);
		}
		iii++;
		fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(job_to_exec->gfr)));
		fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(job_to_exec->gfr), gfc_get_filelen(job_to_exec->gfr));
	}
}
