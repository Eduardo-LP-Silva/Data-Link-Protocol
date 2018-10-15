#ifndef TRANSMITTER_H
#define TRANSMITTER_H

int llwrite(int fd, char * buffer, int length);
int sendFile(char* filename);


#endif