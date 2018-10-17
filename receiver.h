#ifndef RECEIVER_H
#define RECEIVER_H

#include "utilities.h"

int llread(int fd, char * buffer);
char headerCheck(char received[], int size);
int dataCheck(char received[], int size);
int sendAnswer(int fd, char control);
int checkControlDataPacket(char *controlDataPacket, char *filename, int *fileSize);
int readDataPacket(char *dataPacket, applicationLayer *app, char *buffer, char *filename, int *fileSize);


#endif