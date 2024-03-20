#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>

#define PERMS 0644
#define HP 10000000 // Milliseconds for Highest Priority
#define MP 20000000 //Middle Priority
#define LP 40000000 //Lowest Priority


void help(void); //Help function for if the user prints out help.
void incrementClock(int*, int, int*, int); //The system clock used by both programs.
static void firstsignal(int); //First signal is a SIGALRM signal that goes off after 60 seconds pass
static void secondsignal(int); //Second signal is a SIGINT signal that goes off if the user presses CTRL+C
int getRandomsecs(int); //Function designed to get random amount of seconds in the range of 1 and the value of another number.int 
int getRandomnanos(void); //Givs a random amount of nanoseconds
void printTable(int*, FILE*); 

struct PCB 
 {
   int occupied; //If the process is table is occupied
   pid_t pid;
   int startSeconds;
   int startNano;
   int blocked; // whether process is blocked
   int eventBlockedUntilSec; // when will this process become unblocked
   int eventBlockedUntilNano; // when will this process become unblocked
 };
 
struct Queue //Queue struct for processes
{
 int pid;
 double priority;
 
}; 


 
struct PCB processTable[20];

int main(int argc, char** argv)
{
 Queue *q0, *q1, *q2;
}

void help(void) //Help function 
 {
  printf("Program is invoked using './oss [-h] -n # -s # -t # -i # -f filename', where # is any positive integer\n");
  printf("-h is optional: it prints out these instructions and ends the program\n");
  printf("The # after -n is the number of children to be launched it cannot be greater than 20\n");
  printf("The # after -s is the number of children that can be run simultainously\n");
  printf("The # after -t is the maximum bound of time that a child processes can be launched for\n");
  printf("The # after -i is repesents how many milliseconds a child should wait before launching \n");
  printf("filename represents the name of the file that will be used log information about the file\n");
 }
 
 void incrementClock(int* shm, int i, int*nanoholder, int sc)
 {
 
  shm[1] += 1000
 
 
  if(abs(shm[1] - *nanoholder) >= 1000000) //One million nanoseconds is equal to a millisecond.
   {
     shm[2] += 1;
     *nanoholder = shm[1];
     printf("%d\n", shm[2]);
   }
   
  if (shm[2] >= i) //Milliseconds are used to control how often a child is launched.
   {
    childready = true;
    shm[2] = 0; //Millseconds reset after a certain amount of time.
   }
  
  if (shm[1] >= 1000000000) //One billion nanoseconds equals one second.
  {
    shm[0] += 1;
    shm[1] = 0;
  }
 }
 
 int getRandomsecs(int upper) //Prooduces a truly random number of seconds based on the time because of srand
 {
  srand(time(NULL));
  int j = (rand() % (upper - 1 + 1)) + 1;
  return j;
  
 }
 int getRandomnanos(void)
 {
  srand(time(NULL));
  int j = (rand() % (999999999 - 1 + 1)) + 1;
  return j;
 }