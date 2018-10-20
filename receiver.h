#ifndef RECEIVER_H
#define RECEIVER_H

#include "utilities.h"

linkLayer ll;

int stateMachineReceiver(applicationLayer *al, char* device, char *fileSize, char *filename);
int receiveFile(char *device);
int llread(int fd, char *buffer);
int destuff(char* buffer, int* size);
char headerCheck(char received[]);
int sendAnswer(int fd, char control);
int readDataPacket2(int *fd, applicationLayer *app, char *buffer, char *filename, char *fileSize, int packetSize);
int checkControlDataPacket2(int i, char *buffer, char *filename, char *fileSize, int packetSize);
int trailerCheck(char received[], int size);

#endif
