#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <limits.h>
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
void printTable(int*); 
static void firstsignal(int); //First signal is a SIGALRM signal that goes off after 60 seconds pass
static void secondsignal(int); //Second signal is a SIGINT signal that goes off if the user presses CTRL+C
int filenumbercounter(FILE*);
int giveslice(int);
pid_t getpriority(void);
void makeTable(void);
int PCB_Space(void);
void launch(int, int, int, int*);
int numOfChild(int, int);
void pidget(pid_t, int*);
int getIndex(pid_t)

int priority;
int stoptime;
FILE* file;

char str[sizeof(int)]; //Will hold the amount of seconds in a char array
char str2[sizeof(int)]; //Will hold the nanoseconds
int rsecs ,rnanosecs; //The random number of seconds, and nanosecondsfor each child.
struct PCB 
 {
   int occupied; //If the process is table is occupied
   pid_t pid;
   int startSeconds;
   int startNano;
   int timeused; //The amount of remaining time used for a process that decides to terminate early.
   int blocked; // whether process is blocked
   int eventBlockedUntilSec; // when will this process become unblocked
   int eventBlockedUntilNano; // when will this process become unblocked
 };
 
 typedef struct msgbuffer {
 int intData;
 long mType;
 char strData[100];
 } msgbuffer;
 
 
//Queue struct and its associated functions all come from https://www.geeksforgeeks.org/introduction-and-array-implementation-of-queue/
// A structure to represent a queue
struct Queue {
    int front, rear, size;
    unsigned capacity;
    int* array;
};
 
// function to create a queue
// of given capacity.
// It initializes size of queue as 0
struct Queue* createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*)malloc(
        sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
 
    // This is important, see the enqueue
    queue->rear = capacity - 1;
    queue->array = (int*)malloc(
        queue->capacity * sizeof(int));
    return queue;
}
 
// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue)
{
    return (queue->size == queue->capacity);
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}
 
// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)
                  % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
}
 
// Function to remove an item from queue.
// It changes front and size
int dequeue(struct Queue* queue)
{
    if (isEmpty(queue))
        return -1;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)
                   % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}
 
// Function to get front of queue
int front(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->front];
}
 
// Function to get rear of queue
int rear(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->rear];
}

//The four queues; The three priorty based queues, alongside the blocked queue.
   
struct Queue* q0 = createQueue(20);
struct Queue* q1 = createQueue(20);
struct Queue* q2 = createQueue(20); 
struct Queue* qb = createQueue(20); //Blocked Queue

int schedule(pid_t, msgbuffer, int*, int,  int, int, int);
int receive(pid_t, msgbuffer, int*, int,  int, int, int*);
void updateTable(pid_t, msgbuffer, int*, int*);
void block(int*);

struct PCB processTable[20];
bool childready = true; //Will change it's value depending on if a child is ready to launch.
bool got_signal = false; //Signal for when the program ends
int stillChildrenToLaunch(); //Will detect if there are still children to launch in the program.
int childrenStillRunning();

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
  int slot = 0;
  char* filename = NULL;
  
  
  int opt;
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
    
   if(filename == NULL)
   {
    printf("Error: You must enter a file name!\n");
    return 1;
   }
   
   if(file = fopen(filename, "r")) //Checking if the file exists; if it does, the program will terminate.
   {
    printf("That file name is taken; please enter a name that isn't taken already\n");
    return 1;
   }
   
   file = fopen(filename, "w");
    
   shm[0] = 0; //Seconds
   shm[1] = 0; //Nanoseconds
   shm[2] = 0; //Milliseconds
   int nanoholder = shm[1]; //Nanoholder will be used for remembering howmany nanoseconds have passed in each iteration
   int tableget = 0; //Will be used for checking if it is time to print the process table.
   
   
   msgbuffer buf;
   
   makeTable();
   int child = 0;
   signal(SIGALRM, firstsignal); //First signal
   signal(SIGINT, secondsignal); //Second signal
   alarm(60); //Alarm goes off after 60 seconds.
 
   while(stillChildrenToLaunch() || childrenStillRunning())
   {
     
     incrementClock(shm, i, &nanoholder, 0);
     
     if(abs(shm[1] - tableget) >= 500000000) //If half a second passes, the process table will print.
     {
      tableget = shm[1];
      printTable(shm);
     }
     
     launch(s, sc, t, shm);
     
     block(shm);
     
     pid_t priority;
     
     priority = getpriority();
     
     int sendmsg;
     sendmsg = schedule(priority, buf, shm, i, &nanoholder, 500000, msqid);
     
   }
   
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
  
 int giveslice(int priority)
 {
  int slice;
  
  switch(priority)
  {
   case 1:
         slice = HP;
         break;
   case 2:
         slice = MP;
         break;
   case 3: 
         slice = LP;
         break;
  }
  
  return slice;
 }
 
 void printTable(int* shm) //Prints th oss data and data for each of the children.
 {
    printf("OSS PID: %d SysClockS: %d SysClockNano: %d\n", getpid(), shm[0], shm[1]);
    printf("Process Table\n");
    printf("Entry  Occupied  PID  StartS  StartN\n");
   for(int q = 0; q < 20; q++)
    {
     printf("%d       %d      %d   %d   %d\n", (1 + q), processTable[q].occupied, processTable[q].pid, processTable[q].startSeconds,        processTable[q].startNano);
    }
    
 }
 
 pid_t getpriority(void)
 {
   pid_t pid = -1;
   
   if(!isEmpty(q0))
    {
     pid = front(q0);
     priorty = 1;
     dequeue (q0);
     enqueue(q1, (int)pid);
    }
    
   else if(!isEmpty(q1))
    {
     pid = front(q1);
     priority = 2;
     dequeue(q1);
     enqueue(q2, (int)pid);
    }
    
   else if(!isEmpty(q2))
    {
     pid = front(q2);
     priority = 3;
    }
   return pid;
 }
 
 void block(int * shm)
 {
   int entry;
   for(int count = 0; count < 20; count++)
   {
    if(qb->array[count] != -1)
    {
     entry =  getIndex(qb->array[count]);
     
     if(!processTable[entry].occupied)
      continue;
      
     if(shm[0] >= processTable[entry].eventBlockedUntilSec && shm[1] > processTable[entry].eventBlockedUntilNano)
     {
      processTable[entry].blocked = 0;
      processTable[entry].eventBlockedUntilSec = 0;
      processTable[entry].eventBlockedUntilNano = 0;
      
      if(!dequeue(qb, (int)processTable[entry].pid))
      {
       perror("Item not Found\n");
       exit(1);
      }
      
      if(!enqueue(q1, (int)processTable[entry].pid))
      {
       perror("Overflow\n");
       exit(1);
      }
     }
     
    }
   }
 }
 
 
void makeTable(void)
{
 int i = 0;
 for(i; i < 20; i++)
  {
   processTable[i].occupied = 0;
   processTable[i].pid = 0;
   processTable[i].startSeconds = 0;
   processTable[i].startNano = 0;
   processTable[i].timeused = 0;
   processTable[i].blocked = 0;
   processTable[i].eventBlockedUntilSec = 0; 
   processTable[i].eventBlockedUntilNano = 0;
  }
}

int PCB_Space(void)
{
 int i = 0;
 
 for(i; i<20; i++)
 {
  if(processTable[i].occupied == 0)
   return i;
 }
 
 return -1;
}

int stillChildrenToLaunch()
{
 if(processTable[19].pid == 0)
  return 1;
  
  return 0;
}
int childrenStillRunning()
{
 for(int count = 0; count < 20; count++)
 {
  if(processTable[count].occupied)
  return 1;
 }
  return 0;
}

void launch(int s, int sc, int t, int* shm)
{
 if(childready == false)
  return;
  
 if(numOfChild(s, sc) && stillChildrenToLaunch())
 {
  pid_t p;
  p = fork();
  
  if(p < 0)
   {
    perror("Fork error\n");
    exit(1);
   }
  else if(p == 0)
  {
    childready = false;
    rsecs = getRandomsecs(t);
    rnanosecs = getRandomnanos();
    snprintf(str, sizeof(int), "%d", rsecs);
    snprintf(str2, sizeof(int), "%d", rnanosecs);
    execlp("./worker", "./worker", str, str2, NULL);
    exit(1);
  }
  
  else
  {
   pidget(p, shm)
   if(!enqueue(q1, (int)p))
    {
     perror("Error: Coulnd't add child to the high priority queue!\n");
     exit(1);
    }
    
    sc++;
  }
 }
}

int numOfChild(int s, int sc)
{
 if(sc < s)
  return 1;
 return 0;
}

void pidget (pid_t pid, int* shm)
{
 int i;
 i = 0;
 
 while(processTable[i].pid != 0)
  i++;
  
 processTable[i].occupied = 1;
 processTable[i].pid = pid;
 processTable[i].startSeconds = shm[0];
 processTable[i].startNano = shm[1];
 processTable[i].priority = 1;
}

int getIndex(pid_t pid)
{
 for(int count = 0; count < 20; count++)
 {
  if(processTable[count].pid == pid)
   return count;
 }
 return 0;
}

int schedule(pid_t pid, msgbuffer buf, int* shm, int i, int nano, int clock, int msqid)
{
 incrementClock(shm, i, &nano, clock);
 
 if(pid == -1)
 {
  return 0;
 }
 
 buf.mtype = pid;
 if(priority == 1)
  {
   buf.intData = HP;
   stoptime = HP;
  }
  
 else if (priority == 2)
  {
   buf.intData = MP;
   stoptime = HP;
  }
  
 else if(priority == 3)
  {
   buf.intData = LP;
   stoptime = HP;
  }
  
  if (msgsnd(msqid, &buf, sizeof(msgbuffer) - sizeof(long), 0) == -1)
   {
    perror("msgsnd failed\n");
    exit(1);
   }
   
  return 1;
}

int receive(pid_t pid, msgbuffer buf, int* shm, int i,  int nano, int msqid, int* sc)
{
 
 msgbuffer rcvmsg;
 
 if(msgrcv(msqid, &rcvmsg, sizeof(msgbuffer), getpid(), 0) == -1)
 {
  perror("msgrcv to child failed\n");
  exit(1);
 }
 
 incrementClock(shm, i, &nano, rcvmsg.intData);
 updateTable(pid, rcvmsg, &sc, shm);
}

void updateTable(pid_t pid, msgbuffer rcvmsg, int* sc, int* shm)
{
 int entry = getIndex(pid);
 
 if(rcvmsg.strData == 'EARLY')
  {
   processTable[entry].occupied = 0;
   
   if(priority == 1)
    {
     dequeue(q0);
    }
   else if(priority == 2)
    {
     dequeue(q1);
    }
   else if(priority == 3)
   {
    dequeue(q2);
   }
  --sc;
  
  processTable[entry].blocked = 0;
  }
  
 else if(rcvmsg.strData == 'COMPLETE')
  {
   if(pid == front(q0))
    {
     dequeue(q0);
     enqueue(q1, (int)pid);
    }
    
   else if(pid == front(q1))
    {
     dequeue(q1);
     enqueue(q2, (int)pid);
    }
  }
  
 else if(rcvmsg.strData == 'BLOCKED')
 {
  processTable[entry].blocked = 1;
  
  if(pid == front(q0))
   dequeue(q0);
  
  else if(pid == front(q1))
   dequeue(q1);
  
  else if(pid == front(q2))
   dequeue(q2);
   
  enqueue(qb, (int)pid)
  
  processTable[entry].eventBlockedUntilSec = shm[0] + 1;
  processTable[entry].eventBlockedUntilNano = shm[1];
  
 }
}