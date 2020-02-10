
#include <Ctrie.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "print_graph.h"
#include <debuginternal.h>
#include "print_part_graph.h"

#if DEBUG

#include <assert.h>

#endif

#define GOLD_RATIO 11400714819323198485ULL 
#define LRAND_MAX (1ULL<<31) 

size_t limit_sf, limit_r, limit_i;

int test_size, n_threads;

Ctrie *ct;

void *prepare_worker(void *entry_point)
{
  for(int i=0; i<test_size/n_threads; i++){
    size_t rng, value;
    lrand48_r(entry_point, (long int *) &rng);
    //value = rng * GOLD_RATIO;
    value =rng * GOLD_RATIO;
   
    if(rng < limit_r){
      insert( value, value, ct);
    }
  }
  return NULL;
}

void *bench_worker(void *entry_point)
{
  int thread_limit = test_size/n_threads;
  for(int i=0; i<thread_limit; i++){
    size_t rng,value;
    lrand48_r(entry_point, (long int *) &rng);
    value =rng * GOLD_RATIO;
    if(rng < limit_sf){
#if DEBUG
      //assert((size_t)lookup( (int)value, ct)==value);
       if(lookup(value,ct)!=value){
	printf("look:%lu , val:%lu\n", lookup_debug( value,ct), value);
	abort();
      }
#else
      lookup(value, ct);
#endif
    }
    else if(rng < limit_r){
      remover( value, ct);
    }
    else if(rng < limit_i){
      if(insert( value, value, ct)!=1)
	printf("INSERT RETURNED 0\n");
      if(lookup(value,ct)!= value){
	printf("ERROR BENCHING INSERT, BEFORE TESTE lookup:%lu, value:%lu\n", lookup(value, ct), value);
	//insert((int)value,(int)value, ct);
	//insert_debug(value, value, ct);
	//printf("AT END  lookup:%lu, value:%lu\n", lookup(value, ct), value);
	abort();
      }
    }
    else{
#if DEBUG
      assert(lookup(value,ct) == 0);//==NULL
#else
      lookup( value, ct);
#endif
    }
  }
  
  return NULL;
}

#if DEBUG
void *test_worker(void *entry_point)
{
  for(int i=0; i<test_size/n_threads; i++){   
    size_t rng, value;
    lrand48_r(entry_point, (long int *) &rng);
    value = rng * GOLD_RATIO;
    if(rng < limit_sf){
      //assert((size_t)lookup((int)value, ct)==value);
      if(lookup(value,ct)!=value){
	printf("FAILED ON SEARCH TO FOUND\nlook:%lu , val:%lu\n", lookup_debug(value,ct), value);
	abort();
      }
    }
    else if(rng < limit_r){
      // assert(lookup((int)value,ct)==0);//==NULL
       if(lookup(value,ct)!=0){
	 printf("FAILED ON REMOVE TESTE\n look:%lu , val:%lu\n", lookup_debug(value,ct), value);
	abort();
      }
    }
    else if(rng < limit_i){
      //assert((size_t)lookup((int)value,ct)==value);
       if(lookup(value,ct)!=value){
	 printf("FAILED ON INSERT TESTE\nlook:%lu , val:%lu\n", lookup_debug( value,ct), value);
	 if(is_present(value, ct))
	   printf("node with val present on ctrie\n");
	 else
	   printf("node with val not exist\n");
	 abort();
      }
    }
    else{
      //assert(lookup((int)value,ct)==0);//==NULL
       if(lookup(value,ct)!=0){
	 printf("FAILED ON SEARCH NOT FOUND\nlook:%lu , val:%lu\n", lookup_debug(value,ct), value);
	 
	 abort();
      }
    }
  }
 
  return NULL;
}
#endif


int main(int argc, char **argv)
{
  if(argc < 7){
    printf("usage: bench <threads> <nodes> <inserts> <removes> <searches found> <searches not found>\n");
    return -1;
  }
  printf("preparing data.\n");
  n_threads = atoi(argv[1]);
  test_size = atoi(argv[2]);
  size_t inserts = atoi(argv[3]),
    removes = atoi(argv[4]),
    searches_found = atoi(argv[5]),
    searches_not_found = atoi(argv[6]),
    total = inserts + removes + searches_found + searches_not_found;
  limit_sf = LRAND_MAX*searches_found/total;//
  limit_r = limit_sf + LRAND_MAX*removes/total;
  limit_i = limit_r + LRAND_MAX*inserts/total;
  struct timespec start_monoraw,
    end_monoraw,
    start_process,
    end_process;
  double time;
  pthread_t *threads = malloc(n_threads*sizeof(pthread_t));
  struct drand48_data **seed = malloc(n_threads*sizeof(struct drand48_data *));
  ct = new_ctrie();
 
  for(int i=0; i<n_threads; i++)
    seed[i] = aligned_alloc(64, 64);

  
  if(limit_r!=0){
    for(int i=0; i<n_threads; i++){
      srand48_r(i, seed[i]);
      pthread_create(&threads[i], NULL, prepare_worker, seed[i]);
    }
    for(int i=0;i<n_threads; i++){
      pthread_join(threads[i], NULL);
    }
  }
  
  printf("starting test w/ l_sf:%lu, l_r:%lu, l_i:%lu\n",limit_sf, limit_r, limit_i);
  clock_gettime(CLOCK_MONOTONIC_RAW, &start_monoraw);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_process);

  for(int i=0; i<n_threads; i++){
    srand48_r(i, seed[i]);
    pthread_create(&threads[i], NULL, bench_worker, seed[i]);
  }
  for(int i=0; i<n_threads; i++)
    pthread_join(threads[i], NULL);
  
  clock_gettime(CLOCK_MONOTONIC_RAW, &end_monoraw);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_process);
  time = end_monoraw.tv_sec - start_monoraw.tv_sec + ((end_monoraw.tv_nsec - start_monoraw.tv_nsec)/1000000000.0);
  printf("Real time: %lf\n", time);
  time = end_process.tv_sec - start_process.tv_sec + ((end_process.tv_nsec - start_process.tv_nsec)/1000000000.0);
  printf("Process time: %lf\n", time);
  

#if DEBUG
  
  for(int i=0; i<n_threads; i++){
    srand48_r(i, seed[i]);
    pthread_create(&threads[i], NULL, test_worker, seed[i]);
  }
  
  for(int i=0; i<n_threads; i++){
    pthread_join(threads[i], NULL);
  }
//	lfht_check_compression(head);
//	free_lfht_mtu(head);
  printf("Correct!\n");
#endif
  return 0;
}

