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


typedef struct msgbuffer {
 int intData;
 int quantum;
 long mType;
 char strData[100];
 } msgbuffer;
 
int TBC(int, int); //Random number generater that determines if a process will complete it's timeslice, be blocked, or 
int choice(void); //Determines what choice the process will make based on TBC
int timeused(msgbuffer);
int unblocktime(int*);
 
int main(int argc, char **argv)
{
 msgbuffer buf;
 int n = atoi(argv[1]); //Will count the seconds used for the system clock.
 int m = atoi(argv[2]); //Will count the nano seconds used for the system clock.
 int rcvtime; //Time that will be received by oss.c
    
 pid_t pid = getpid();
 pid_t ppid = getppid();
 buf.mType = 1;
 buf.intData = 0;
 
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

   int i = 1; 
   int stopsecs = shm[0] + n; //Stop time in seconds.
   int stopnanosecs = shm[1] + m; //Stop time for nanosecs
   
   while(shm[0] < stopsecs || shm[1] < stopnanosecs)
    {
     int msgwait = 0;
     
     while(!msgwait)
      {
       if(msgrcv(msqid, &buf, sizeof(msgbuffer), pid, 0) >= 0)
       {
        msgwait = 1;
       }
      }
      
      int decide = choice();
      
      if(decide == 2)
      {
       buf.intData = timeused(buf);
       if(msgsnd(msqid, &buf, sizeof(msgbuffer) - sizeof(long), 0) == -1)
       {
         perror("Error: Failed to warn of blocking\n");
         exit(1);
       }
       
      if(msgrcv(msqid, &buf, sizeof(msgbuffer), pid, 0) == -1)
      {
       perror("Error: Failed to get the unblocking time\n!");
       exit(1);
      }
      
      while(1)
       {
        if(shm[1] >= atoi(buf.strData))
         break;
       }
      }
      
      else if (decide == 3)
      {
       buf.intData = -timeused(buf);
       if(msgsnd(msqid, &buf, sizeof(msgbuffer) - sizeof(long), 0) == -1)
       {
        perror("Failed to warn about early termination!\n");
        exit(1);
       }
       break;
      }
      
      if(decide == 1)
      {
       buf.intData = buf.quantum;
       if(msgsnd(msqid, &buf, sizeof(msgbuffer) - sizeof(long), 0) == -1)
       {
        perror("Failed to warn about complete quantum!\n");
        exit(1);
       }
      }

    }
    
   shmdt(shm); //Freeing shared memory.
   return 0;
    
}


int TBC(int max, int min)
{
   srand(time(NULL));
   int j = (rand() % (max - min + 1)) + 1;
   return j; 
} 

int choice(void)
{
 int action = TBC(100, 0);
 if(action < 70)
  return 1;
 if(action <= 99)
  return 2;
 return 3;
}

int timeused(msgbuffer buf)
{
 int time = TBC((buf.quantum - 1), 1);
 return time;
}

int unblocktime(int* shm)
{
 
}