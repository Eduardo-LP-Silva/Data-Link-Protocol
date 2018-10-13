#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400 /* bit rate*/
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define BUFFER 255
#define FLAG 0x7E
#define ADDR 0x03
#define SET_C 0x03
#define DISC_C 0x0B
#define UA_C 0x07
#define RR_C 0x06
#define REJ_C 0x01
#define ESCAPE 0x7d
#define TRANSMITTER 0
#define RECEIVER 1

//Global Variables
int timeoutSize = 3;

//General Utility Functions
void sigalrm_handler(int signal);
void printArray(char* arr, int length);
int stateMachine(char received[], int C);
void swap(char* a, char*b);
int abs(int a);
void shiftRight(char* buffer, int size, int position, int shift);
void shiftLeft(char* buffer, int size, int position, int shift);

//Application - Data Protocol Interface
int llopen(int fd, int flag);
int llwrite(int fd, char * buffer, int length);
int llread(int fd, char * buffer);

//Application Protocol
int dataCheck(char received[], int size);
int sendAnswer(int fd, char control);

// Data Protocol
int headerCheck(char received[], int size);
int messageCheck(char received[]);
