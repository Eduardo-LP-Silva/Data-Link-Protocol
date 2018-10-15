#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>



//Global Variables
int timeoutSize = 3;

//General Utility Functions
void sigalrm_handler(int signal);
int stateMachine(char received[], int C);

/*
//Application - Data Protocol Interface
int llopen(int fd, int flag);
int llwrite(int fd, char * buffer, int length);
int llread(int fd, char * buffer);
*/

//Application Protocol
int dataCheck(char received[], int size);
int sendAnswer(int fd, char control);

// Data Protocol
int headerCheck(char received[], int size);
int messageCheck(char received[]);
