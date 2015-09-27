#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>


#include "gfserver.h"
#include "content.h"
#include "steque.h"

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  webproxy [options]\n"                                                      \
"options:\n"                                                                  \
"  -p                  Listen port (Default: 8888)\n"                         \
"  -t                  Number of threads (Default: 1)\n"                      \
"  -c                  Content file mapping keys to content files\n"          \
"  -h                  Show this help message\n"                              

ssize_t handler_get(gfcontext_t *ctx, char *path, void* arg);
extern ssize_t handler_worker();

//mutli threaded constants
steque_t* queue;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_cons = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_proc = PTHREAD_COND_INITIALIZER;


/* Main ========================================================= */
int main(int argc, char **argv) {
  int option_char = 0;
  unsigned short port = 8888;
  char *content = "content.txt";
  int nthreads = 1;
  gfserver_t *gfs;

  // Parse and set command line arguments
  while ((option_char = getopt(argc, argv, "p:t:c:h")) != -1) {
    switch (option_char) {
      case 'p': // listen-port
        port = atoi(optarg);
        break;
      case 'c': // file-path
        content = optarg;
        break;           
      case 't': // nthreads
        nthreads = atoi(optarg);
        break;                         
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;       
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
    }
  }
  
  content_init(content);

  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_port(gfs, port);
  gfserver_set_maxpending(gfs, 100);
  gfserver_set_handler(gfs, handler_get);
  gfserver_set_handlerarg(gfs, NULL);

  //mtimplemnation
  queue = malloc(sizeof(steque_t));
  steque_init(queue);
  //create nthreads threads
  pthread_t *thread_list = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
  for (int ii =0; ii < nthreads; ii++){
	  pthread_create(thread_list + ii, NULL, (void *)&handler_worker, NULL);
  }
  /*Loops forever*/
  gfserver_serve(gfs);
}
