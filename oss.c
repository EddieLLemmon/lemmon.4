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
static void firstsignal(int); //First signal is a SIGALRM signal that goes off after 60 seconds pass
static void secondsignal(int); //Second signal is a SIGINT signal that goes off if the user presses CTRL+C
int filenumbercounter(FILE*);

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
 
struct MLFQ //Queue struct for processes
{
 int priority;
 pid_t pid;
 int wait_time;
 int burst_time;
 int turn_time;
 int remain_time;
 int arrival_time;
}; 


 
struct PCB processTable[20];

int main(int argc, char** argv)
{
  int n = 0; //Number of children to be produced.
  int s = 0; //Number of simultainious programs that can run
  int t = 0; //Maximum number of seconds each child can run for.
  int i = 0; //Amount of time between each child launch
  int m = 0; //Counts how many children have terminated
  int nc = 0; //Keeps tract of the next child in the message queue.
  int sc = 0; //Counts how many children are currently running
  int finished = 0;
  char* filename;
  FILE* file;
  int opt;
  int rsecs ,rnanosecs; //The random number of seconds, and nanosecondsfor each child.
  const int key = ftok("./oss.c", 0); //Key for the program.
  int shmid = shmget(key, sizeof(int) * 4, 0666|IPC_CREAT);
  if (shmid <= 0)
   {
    printf("Error in SHMGET\n");
    exit(1);
   }
   
   int *shm; //The shared memory id is an array because the system clock holds multiple values.
   shm  = shmat(shmid, 0, 0);
   if (shm <= 0)
    {
     perror("Error in shmat!\n");
     exit(1);
    }
    
    int msqid;
    key_t key2;
    system("touch msgq.txt");
    if ((key2 = ftok("msgq.txt", 1)) == -1) {
    
    perror("ftok error in message queue!\n");
    exit(1);
    }
    
    if ((msqid = msgget(key2, PERMS | IPC_CREAT)) == -1) {
    
    perror("msgget in parent!\n");
    exit(1);
    }
    
   
    
    while ((opt = getopt(argc, argv, ":hn:s:t:i:f:")) != -1)
    {
      switch (opt)
       {
         case 'h': //If option h is chosen, it will display the message and end the program.
              help();
              return 1;
              break;
         case 'n': //optarg will be the value of the respective command line argument.
              n = atoi(optarg);
              break;
         case 's':
              s = atoi(optarg);
              break;
         case 't':
              t = atoi(optarg);
              break;
         case 'i':
              i = atoi(optarg);
              break;
         case 'f':
              filename = optarg;
              break;
         case '?':
              printf("Unknown option: %c\n", optopt); //If the user enters an invalid option, the program will not start.
              return 1;
              break;
         case ':':
              printf("Missing arg for %c\n", optopt); //If the option is empty, the program will not start.
              return 1;
              break;
       }
    }
    
   if (n <= 0) //If the user enters an argument of 0 or less for an option, the program will not start.
    {
     printf("Error: Number of child processes cannot be less than!\n");
     return 1;
    }
   if (s <= 0)
    {
     printf("Error: Number of maximum simultainious processes cannot be less than one!\n");
     return 1;
    }
   
   if (t <= 0)
    {
     printf("Error: Maximum bound of time for each process cannot be less than one!\n");
     return 1;
    }
   if (i <= 0)
    {
     printf("Error: Number of milliseconds between child process cannot be less than one!\n");
     return 1;
    }
   
   struct MLFQ *q0, q1, q2;
   
   file = fopen(filename, "w");
    
   shm[0] = 0; //Seconds
   shm[1] = 0; //Nanoseconds
   shm[2] = 0; //Milliseconds
   int nanoholder = shm[1]; //Nanoholder will be used for remembering howmany nanoseconds have passed in each iteration
   int tableget = 0; //Will be used for checking if it is time to print the process table.
   char str[sizeof(int)]; //Will hold the amount of seconds in a char array
   char str2[sizeof(int)]; //Will hold the nanoseconds
   
   signal(SIGALRM, firstsignal); //First signal
   signal(SIGINT, secondsignal); //Second signal
   alarm(60); //Alarm goes off after 60 seconds.
   
   fclose(file);
   shmdt(shm); //Detaching shared memory
   shmctl(shmid,IPC_RMID,NULL); //Freeing memory
   if (msgctl(msqid, IPC_RMID, NULL) == -1){
    perror("msgctl failed!\n");
    exit(1);
   }
   
   return 0;
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
 
  shm[1] += 10000 + sc;
 
 
  if(abs(shm[1] - *nanoholder) >= 1000000) //One million nanoseconds is equal to a millisecond.
   {
    for(int i = 0; i < (shm[1] / 1000000); i++)
     shm[2] += 1;
     *nanoholder = shm[1];
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
 
 int filenumbercounter(FILE* file)
 {
  int count = 0;
  char c;
  for (c = getc(file); c != EOF; c = getc(file))
   {
    if (c == '\n')
    count++;
   }
   
   return count;
 }
 
  static void firstsignal(int s) //First signal
  {
   printf("\n60 SECONDS HAVE PASSED: ELIMINATING ALL PROCESSES AND ENDING PROGRAM\n");
   got_signal = true;
  }
  
   static void secondsignal(int s) //Second signal
  {
   
   printf("\nPROGRAM ENDED: ELIMINATING ALL PROCESSES AND ENDING PROGRAM\n");
   got_signal = true;
  }