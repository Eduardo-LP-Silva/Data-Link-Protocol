#ifndef RECEIVER_H
#define RECEIVER_H

#include "utilities.h"

int stateMachineReceiver(char* device, char *fileSize, char *filename);
int receiveFile(char *device);
int llread(int fd, char *buffer);
char headerCheck(char received[]);
int dataCheck(char received[], int size);
int sendAnswer(int fd, char control);
int readDataPacket(applicationLayer *app, char *buffer, char *filename, int *fileSize, int* packetSize);
int readDataPacket2(int *fd, applicationLayer *app, char *buffer, char *filename, char *fileSize, int packetSize);
int checkControlDataPacket(int fd, char *filename, int *fileSize);
int checkControlDataPacket2(int i, char *buffer, char *filename, char *fileSize, int packetSize);


#endif
