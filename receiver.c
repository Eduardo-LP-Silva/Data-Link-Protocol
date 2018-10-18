#include "receiver.h"

#include "constants.h"
#include "transmitter.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int destuff(char* buffer, int* size)
{
	int i;
	for (i = 0; i < *size; i++) // Destuffs the data package
	{
		if (buffer[i] == ESCAPE)
		{
			if (buffer[i+1] == 0x5e)
			{
				shiftLeft(buffer, *size, i+1, 1);
				(*size)--;

				buffer[i] = FLAG;
			}
			else if (buffer[i+1] == 0x5d)
			{
				shiftLeft(buffer, *size, i+1, 1);
				(*size)--;

				buffer[i] = ESCAPE;
			}
		}
	}

	return 0;
}

int stateMachineReceiver(char* device, int *fileSize, char *filename)
{
	applicationLayer al;
	al.status = 0;
	al.flag = RECEIVER;
	al.dataPacketIndex = 0;
	char dataRead[128*2 + 6];
	int packetSize;
	int fd;

	while (1)
	{
		if (al.status == 0) // Closed
		{
			al.fileDescriptor = openPort(device, al.flag);
			
			if (al.fileDescriptor > 0)
			{
				al.status = 1;
				al.dataPacketIndex = 0;
			}

			printf("Open for connection\n");
		}
		else if (al.status == 1) // Transfering
		{
			
			if(llread(al.fileDescriptor, dataRead) < 0)
			{
				sendAnswer(al.fileDescriptor, REJ_C);
				printf("Error in llread\n");
				continue;
			}
			
			if(readDataPacket2(&al, dataRead, filename, fileSize, &packetSize) < 0)
			{
				sendAnswer(al.fileDescriptor, REJ_C);
				printf("Error in Data Packet\n");
				continue;
			}

			destuff(dataRead, &packetSize);

			// printf("fileSize = %i\n", *fileSize);
			printf("packetSize = %i\n", packetSize);

			char bcc2, flag;

			read(al.fileDescriptor, &bcc2, 1);
			read(al.fileDescriptor, &flag, 1);

			printf("BCC2 = %d\n", bcc2);
			printf("Flag = %d\n", flag);


			if (al.dataPacketIndex > 0)
				write(fd, dataRead, packetSize);
			else
				fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0777);
			
			al.dataPacketIndex++;
			
			packetSize = 0; // Clears dataRead array

			printf("Received Packet\n");
		}
		else if (al.status == 2) // Closing
		{
			
			break;
		}
	}
}

int receiveFile(char *device)
{
	char filename[100];
	int fileSize = 0;

	//Ciclo
	stateMachineReceiver(device, &fileSize, filename);

	return 0;	
}


int llread(int fd, char * buffer)
{
	int i, j, numBytes = 1, receivedSize = 0;
	
	while(1)	// Reads the first 4 bytes of the frame.
	{
		numBytes = read(fd, &buffer[receivedSize++], 1);

		if (receivedSize > 1 && buffer[receivedSize-1] == FLAG)
			break;

	}

	printArray(buffer, receivedSize);

	if (headerCheck(buffer) < 0)
	{
		printf("Error on header");
		return -1;
	}

	return 0;
}

char headerCheck(char received[])
{
	char control, bcc1;
	int i;

	if (received[0] == FLAG && received[1] == ADDR)
	{
		control = received[2];
		bcc1 = received[3];

		if (bcc1 != received[1] ^ control)
			return -1;
	}

	return control;
}

int dataCheck(char received[], int size)
{
	char bcc2;
	int i;

	for (i = 0; i < size-1; i++)
	{
		if (i == 0)
			bcc2 = received[i];
		else
			bcc2 ^= received[i];
	}

	if (bcc2 == received[size-1])
		return 0;

	return -1;	//Error
}

int sendAnswer(int fd, char control)
{
	char buffer[5];
	
	buffer[0] = FLAG;
	buffer[1] = ADDR;
	buffer[2] = control;
	buffer[3] = buffer[1] ^ buffer[2];
	buffer[4] = FLAG;
	
	int written = write(fd, buffer, 5);
	
	if (written < 0)
		printf("Error sending RR!\n");

	return written;
}

int readDataPacket2(applicationLayer *app, char *buffer, char *filename, int *fileSize, int* packetSize)
{	
	int i = 4;
	char controlByte = buffer[i++];

	if (controlByte == 2) // Start
	{
		if (buffer[i++] == 0) // File size
		{
			printf("i = %i\n", i);

			memcpy(fileSize, buffer+i+1, buffer[i]);
			i += buffer[i];
		}

		if (buffer[i++] == 1) // File name
		{
			printf("i = %i\n", i);

			memcpy(fileSize, buffer+i+1, buffer[i]);
			i += buffer[i];
		}

	}
	else if (controlByte == 1) // Data
	{
		char sequenceNumber = buffer[i++];
		printf("sequenceNumber = %i\n", sequenceNumber);
		printf("app->dataPacketIndex = %i\n", app->dataPacketIndex);

		if(sequenceNumber != app->dataPacketIndex - 1)
		{
			printf("Sequence error\n");
			return -1;
		}
	
		unsigned char l1 = buffer[i++], l2 = buffer[i++];
	
		*packetSize = 256 * l2 + l1;
	
		printf("packetSize = %d\n", *packetSize);

		memcpy(buffer, buffer, *packetSize);

	}
	else if (controlByte == 3) // End
	{
		if (buffer[i] == 0) // File size
		{
			i++;
			i += read(app->fileDescriptor, fileSize, buffer[i]);
		}

		if (buffer[i] == 1) // File name
		{
			i++;
			i += read(app->fileDescriptor, filename, buffer[i]);
		}
	}

	return 0;
}

int readDataPacket(applicationLayer *app, char *buffer, char *filename, int *fileSize, int* packetSize)
{
	char controlByte;

	read(app->fileDescriptor, &controlByte, 1);

	int error = -1;

	switch(controlByte)
	{
		case 1:
			break;

		case 2:
			if((error = checkControlDataPacket(app->fileDescriptor, filename, fileSize)) == -1)
				return error;

			return 0;

		case 3:
			if((error = checkControlDataPacket(app->fileDescriptor, filename, fileSize)) == -1)
				return error;

			//TODO SMT
			
			app->status = 2;
			
			return 0;

		default:
			return -1;
			break;
	}

	char sequenceNumber; 
	read(app->fileDescriptor, &sequenceNumber, 1);

	printf("sequenceNumber = %i\n", sequenceNumber);
	printf("app->dataPacketIndex = %i\n", app->dataPacketIndex);

	if(sequenceNumber != app->dataPacketIndex - 1)
	{
		printf("Sequence error\n");
		return -1;
	}

	
	unsigned char l1, l2;
	
	read(app->fileDescriptor, &l2, 1);
	read(app->fileDescriptor, &l1, 1);
	
	*packetSize = 256 * l2 + l1;
	
	printf("packetSize = %d\n", *packetSize);

	read(app->fileDescriptor, buffer, *packetSize);
	//printArray(buffer, *packetSize);

	return 0;
}

int checkControlDataPacket(int fd, char *filename, int *fileSize)
{
	int i, j;
	char T, L;
	int readBytes;

	for(i = 0; i < 2; i++)
	{
		read(fd, &T, 1);
		read(fd, &L, 1);

		printf("T = %d\n", T);
		printf("L = %d\n", L);

		if (T == 0)
		{
			for (j = 0; j < L; j++)
			{
				int coco = read(fd, fileSize+j, 1);;
				printf("coco = %i\n", coco);
			}

			printf("Size = %d\n", *fileSize);
		}
		else
		{
			if(T == 1)
			{
				readBytes = read(fd, filename, L);
			}
			else
				printf("Error in reading L. L = %d\n", L);
		}
	}

	printf("%s\n", filename);

	return 0;	
}


