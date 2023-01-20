/*
 * Producer / Consumer problem
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define BUFFER "./buffer"

/* This declaration is *MISSING* is many solaris environments.
   It should be in the <sys/sem.h> file but often is not! If 
   you receive a duplicate definition error message for semun
   then comment out the union declaration.
   */

union semun {
  int              val;
  struct semid_ds *buf;
  ushort          *array; 
};

main(int argc, char *argv[])
{
  FILE           *fptr;
  static struct sembuf acquire = {0, -1, SEM_UNDO}, 
                       release = {0, 1, SEM_UNDO};
  
  pid_t           c_pid;
  key_t           ipc_key;
  static ushort   start_val[2] = {1, 0};
  int             semid, producer = 0, i, n, p_sleep, c_sleep;
  union semun     arg;
  enum { READ, MADE };
  
  if ( argc != 2 ) {
    fprintf(stderr, "%s sleep_time\n", argv[0]);
    exit(-1);
  }
  
  ipc_key = ftok(".", 'S');
  
  /*
   * Create the semaphore
   */

  if ( (semid = semget(ipc_key, 2, IPC_CREAT | 
		       IPC_EXCL | 0660)) != -1 ) {
    producer = 1;
    arg.array = start_val;
    
    if ( semctl(semid, 0, SETALL, arg) == -1 ) {
      perror("semctl -- producer -- initialization");
      exit(1);
    }
  }
  else if ( (semid = semget(ipc_key, 2, 0)) == -1 ) {
    perror("semget -- consumer -- obtaining semaphore");
    exit(2);
  }
  
  switch ( producer ) {
  case 1:
    p_sleep = atoi(argv[1]);
    srand((unsigned) getpid());
    
    for ( i = 0; i < 10; i++ ) {
      sleep(p_sleep);
      n = rand() % 99 + 1;
      printf("A. The number [%d] generated by producer\n", n);
      acquire.sem_num = READ;
      
      if ( semop(semid, &acquire, 1) == -1 ) {
	perror("semop -- producer -- waiting for consumer to read number");
	exit(3);	
      }
      
      if ( (fptr = fopen(BUFFER, "w")) == NULL ) {
	perror("BUFFER");
	exit(4);
      }
      
      fprintf(fptr, "%d\n", n);
      fclose(fptr);
      
      release.sem_num = MADE;
      
      printf("B. The number [%d] deposited by producer\n", n);
      
      if ( semop(semid, &release, 1) == -1 ) {
	perror("semop -- producer -- indicating new number has been made");
	exit(5);	
      }
    }
    sleep(5);
    
    if ( semctl(semid, 0, IPC_RMID, 0) == -1 ) {
      perror("semctl -- producer");
      exit(6);
    }
    
    printf("Semaphore removed\n");
    break;
    
  case 0:
    c_sleep = atoi(argv[1]);
    c_pid = getpid();
    
    while ( 1 ) {
      sleep(c_sleep);
      acquire.sem_num = MADE;
      
      if ( semop(semid, &acquire, 1) == -1 ) {
	perror("semop -- consumer -- waiting for new number to be made");
	exit(7);	
      }
      
      if ( (fptr = fopen(BUFFER, "r")) == NULL ) {
	perror("BUFFER");
	exit(8);
      }
      
      fscanf(fptr, "%d\n", &n);
      fclose(fptr);
      
      release.sem_num = READ;
      
      if ( semop(semid, &release, 1) == -1 ) {
	perror("semop -- consumer -- indicating number has been read");
	exit(9);
      }
      printf("C. The number [%d] obtained by consumer %6d\n", n, c_pid);
    }
  }
  exit(0);      
}