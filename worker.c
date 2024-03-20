//This is a child program that is designed to print out user information via system clock

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <time.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PERMS 0644

int termearly(void);

int timeslice(int);

typedef struct msgbuffer {
 int intData;
 long mType;
 char strData[100];
 } msgbuffer;
 
int main(int argc, char **argv)
{
 int n = atoi(argv[1]); //Will count the seconds used for the system clock.
 int m = atoi(argv[2]); //Will count the nano seconds used for the system clock.
    
 pid_t pid = getpid();
 pid_t ppid = getppid();
 buf.mType = pid;
 buf.intData = pid;
 
   if (argc != 3) //If the argument count is anything other than 3, the program will cancel.
    {
     printf("Error: Must enter two command line arguments!\n");
     exit(0);
    } 
    
    if (n <= 0 || m <= 0) //User cannot enter non-positive integers as command line arguments.
    {
     printf("Error: command line arguments must be positive integers!\n");
     exit(0);   
    }
    
    const int key = ftok("./oss.c", 0); //The file  used for sharing memory.
    int shmid = shmget(key, sizeof(int) * 4, 0666); //Allocated memory associated with key.
    
    if (shmid <= 0)
     {
      printf("Error with SHMGET in child\n");
      exit(1);
     }
     
    int *shm = shmat(shmid, 0, 0); //Attaching key to memory

    if (shm <= 0)
     {
      printf("Error with shmat\n");
      exit(1);
     }
     
    int msqid = 0;
    key_t key2;
    if ((key2 = ftok("msgq.txt", 1)) == -1) {
    
    perror("ftok error in message queue!\n");
    exit(1);
    }
    
    if ((msqid = msgget(key2, PERMS)) == -1) {
    
    perror("msgget in child!\n");
    exit(1);
    }
    
    
}


int termearly(void)
{
   srand(time(NULL));
   int j = (rand() % (100 - 1 + 1)) + 1;
   return j; 
} 

int timeslice(int slice)
{
 srand(time(NULL));
 int j = (rand() % (slice - 1 + 1)) + 1;
 return j;
}