#ifndef UTILITIES_H
#define UTILITIES_H

#include <sys/types.h>
#include <termios.h>

typedef struct
{
	int fileDescriptor; /*Descritor correspondente à porta série*/
	int status; /*TRANSMITTER | RECEIVER*/
	char dataPacketIndex; //Data Packet Number
} applicationLayer;

struct termios oldtio,newtio;

void swap(char* a, char*b);
int abs(int a);
void shiftRight(char* buffer, int size, int position, int shift);
void shiftLeft(char* buffer, int size, int position, int shift);
void printArray(char* arr, int length);
int stateMachine(char received[], int C);
int messageCheck(char received[]);
int openPort(char* device, int flag);
int llopen(int fd, int flag);
int llclose(int fd);

#endif
