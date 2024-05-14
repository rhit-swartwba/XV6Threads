#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include <stddef.h>
#include "kernel/riscv.h"

#define NUM_THREADS 4

int value = 0;
int arr[NUM_THREADS+1];
int *addr;

int thread_create(void* (*fnc)(void *), int* pid, void* arg)
{
  void* stack = malloc(PGSIZE);
  return tcreate(fnc, pid, arg, stack);
}

int thread_join(int pid)
{
  uint64 stack_addr;
  int retVal = tjoin(pid, (uint64)&stack_addr);
  free((void*)stack_addr);
  return retVal;
}

void* threadtest1(void *arg) {
  int* num = (int*)arg;
	tlock();
  printf("Thread %d is working \n", *num);
	tunlock();
  arr[*num] = *num;
  sleep(20);
	tlock();
  printf("Thread %d num finished\n", *num);
	tunlock();
  exit(0);
}

void* threadtest2(void *arg) {
	
  int* num = (int*)arg;
  tlock();
  printf("Thread %d is working \n", *num);
  tunlock();

  tlock();
	sbrk(PGSIZE);
  tunlock();

  arr[*num] = *num;
  sleep(20);
  tlock();
	sbrk(-PGSIZE);
  tunlock();
  tlock();
  printf("Thread %d num finished\n", *num);
  tunlock();
  exit(0);
}

void* threadtest3(void *arg) {
  int* num = (int*)arg;
  tlock();
  printf("Thread %d is working \n", *num);
  tunlock();
  arr[*num] = *num;
  //will sleep longer than the parent does
  sleep(50);
  tlock();
  printf("Thread %d num finished\n", *num);
  tunlock();
  exit(0);
}

void* reader(void *arg) {
  for(int i = 0; i < 5; i++) {
    sleep(12);
    tlock();
    printf("Reader reading value is %d\n", value);
    tunlock();
  }
  exit(0);
}

void* writer(void *arg) {
  for(int i = 0; i < 5; i++) {
    value++;
    tlock();
    printf("Writer incrementing value to %d\n", value);
    tunlock();
    sleep(10);
  }
  exit(0);
}

void* reader_malloc(void *arg) {
	tlock();
	printf("Malloc reader has arrived\n");
	printf("The number read from %p is %d\n", addr, *addr);
	free(addr);
	tunlock();
	exit(0);
}

void* writer_malloc(void *arg) {
	tlock();
	printf("Malloc writer has arrived\n");
	int *ptr = malloc(sizeof(int));
	addr = ptr;
	*addr = 7;
	printf("Just malloced set the value at %p to be %d\n", ptr, *ptr);
	tunlock();
	exit(0);
}

void test1() {
  int pids[NUM_THREADS+1];
  int args[NUM_THREADS+1];
  for(int i = 1; i < NUM_THREADS+1; i++) {
    args[i] = i;
    thread_create(threadtest1, &pids[i], &args[i]);
  }
  for(int i = 1; i < NUM_THREADS+1; i++) {
    thread_join(pids[i]);
		tlock();
    printf("Value at array %d is %d\n", i, arr[i]);
		tunlock();
  }
}

void test2() {
  int pids[NUM_THREADS+1];
  int args[NUM_THREADS+1];
  for(int i = 1; i < NUM_THREADS+1; i++) {
    args[i] = i;
    thread_create(threadtest2, &pids[i], &args[i]);
  }
  for(int i = 1; i < NUM_THREADS + 1; i++) {
    thread_join(pids[i]);
		/*
    tlock();
    printf("Value at array %d is %d\n", i, arr[i]);
    tunlock();
		*/
  }
}


void test3() {
  int pids[NUM_THREADS+1];
  int args[NUM_THREADS+1];
  for(int i = 1; i < NUM_THREADS+1; i++) {
    args[i] = i;
    thread_create(threadtest3, &pids[i], &args[i]);
  }
  sleep(20);
  tlock();
  printf("Letting the main process die without the threads finishing and joining\n");
  tunlock();
  exit(0);
  //do not join back to check if main process will kill children threads
}

void test4() {
 int pids[2];
 thread_create(writer, &pids[0], NULL);
 thread_create(reader, &pids[1], NULL);
 thread_join(pids[0]);
 thread_join(pids[1]);
 exit(0);
}

void test5() {
	int pids[2];
 thread_create(writer_malloc, &pids[0], NULL);
 sleep(10);
 thread_create(reader_malloc, &pids[1], NULL);
 thread_join(pids[0]);
 thread_join(pids[1]);
 exit(0);
}

int main(int argc, char *argv[]) {
  int testNum = 1;
  if(argc == 2) {
    testNum =  atoi(argv[1]);
  }
  if(argc > 2) {
    printf("Invalid test number\n");
    return -1;
  }
  switch(testNum) {
    case 2:
      printf("Running test 2: Each process allocates a new page using sbrk\n");
			test2();
      break;
    case 3:
      printf("Running test 3: Main thread will exit before the threads finish running\n");
      test3();
      break;
    case 4:
      printf("Running test 4: One reader and one writer thread to clearly show modification of variables\n");
      test4();
      break;
    case 5:
      printf("Running test 5: MALLOC - One reader and one writer thread to clearly show modification of variables\n");
      test5();
      break;
    default:
      printf("Running test 1: Modifying the same global array\n");
      test1();
      break;
  }
  exit(0);
}
