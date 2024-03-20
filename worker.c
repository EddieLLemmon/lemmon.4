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

typedef struct msgbuffer {
 int intData;
 long mType;
 char strData[100];
 } msgbuffer;
 
int main(int argc, char **argv)
{
 
}


int termearly(void)
{
   srand(time(NULL));
   
   
} 