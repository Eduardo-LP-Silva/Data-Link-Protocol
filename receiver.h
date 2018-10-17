#ifndef RECEIVER_H
#define RECEIVER_H

#include "utilities.h"

int stateMachineReceiver(char* device, char* buffer, int *size, char *filename);
int receiveFile(char *device);
int llread(int fd, char * buffer);
char headerCheck(char received[], int size);
int dataCheck(char received[], int size);
int sendAnswer(int fd, char control);
int readDataPacket(applicationLayer *app, char *buffer, char *filename, int *fileSize, int* packetSize);
int checkControlDataPacket(int fd, char *filename, int *fileSize);


#endif
