#ifndef RECEIVER_H
#define RECEIVER_H


int llread(int fd, char * buffer);
int headerCheck(char received[], int size);
int dataCheck(char received[], int size);
int messageCheck(char received[]);
int sendAnswer(int fd, char control);

#endif