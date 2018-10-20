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

int receiveFile(char *device)
{
	char filename[100];
	char fileSize[10];
	applicationLayer al;

	ll.baudRate = BAUDRATE;
	ll.sequenceNumber = 0;
	ll.numTransmissions = 0;

	//Ciclo
	stateMachineReceiver(&al, device, fileSize, filename);

	return 0;	
}

int stateMachineReceiver(applicationLayer *al, char* device, char *fileSize, char *filename)
{
	al->status = 0;
	al->flag = RECEIVER;
	al->dataPacketIndex = 0;
	
	char* dataRead = malloc(128*2 + 6);
	int packetSize;
	int fd;

	while (1)
	{
		if (al->status == 0) // Closed
		{
			al->fileDescriptor = openPort(device, al->flag);
			
			// al.fileDescriptor = open(device, O_RDONLY);
			
			if (al->fileDescriptor > 0)
			{
				al->status = 1;
				al->dataPacketIndex = 0;
			}

			printf("Open for connection\n");
		}
		else if (al->status == 1) // Transfering
		{
			packetSize = llread(al->fileDescriptor, dataRead);

			printf("packetSize = %i\n", packetSize);

			if(packetSize < 0)
			{
				sendAnswer(al->fileDescriptor, REJ_C);
				printf("Error in llread\n");
				continue;
			}

			if(readDataPacket2(&fd, al, dataRead, filename, fileSize, packetSize) < 0)
			{
				sendAnswer(al->fileDescriptor, REJ_C);
				printf("Error in Data Packet\n");
				continue;
			}

			al->dataPacketIndex++;
			
			packetSize = 0; // Clears dataRead array

			printf("Received Packet\n");

			sendAnswer(al->fileDescriptor, RR_C);
		}
		else if (al->status == 2) // Closing
		{
			free(dataRead);

			close(fd);

			break;
		}
	}

	return 0;
}

int llread(int fd, char * buffer)
{
	int i, j, numBytes = 1, receivedSize = 0;
	char temp[128*2 + 6];
	
	while(1)
	{
		numBytes = read(fd, &temp[receivedSize++], 1);

		if (receivedSize > 1 && temp[receivedSize-1] == FLAG)
			break;
	}

	//printf("--------------- What was literally read -------------------\n");
	//printArray(buffer, receivedSize);
	printf("------------------------------------------------------------\n");

	if(headerCheck(temp) < 0)
	{
		printf("Error on header");
		return -1;
	}

	int dataPacketsSize = receivedSize - 6;

	if(dataPacketsSize > 128 * 2)
	{
		printf("Too much data incoming\n");
		return -1;
	}

	dataPacketsSize++; 
	destuff(temp + 4, &dataPacketsSize);
	dataPacketsSize--;

	if(trailerCheck(temp + 4, dataPacketsSize + 2) < 0)
	{
		printf("Error in Trailer\n");
		return -1;
	}

	memcpy(buffer, temp + 4, dataPacketsSize);

	
	//printf("Data Packet size after destuffing: %d\n", dataPacketsSize);
	//ATTENTION: The information beyond dataPacketsSize will be untuched, remaining the same as when the buffer was first read

	return dataPacketsSize; //Application must not know frame structure, thus only the size of the data packets is needed
}

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

int trailerCheck(char received[], int size)
{
	char bcc2;
	int i;

	for (i = 0; i < size-2; i++) 
	{		
		if (i == 0)
			bcc2 = received[i];
		else
			bcc2 ^= received[i];
	}

	if(bcc2 != received[size-2])
		return -1;

	if(received[size - 1] != FLAG)
		return -1;
}

char headerCheck(char received[])
{
	char control, bcc1;
	int i;

	if (received[0] == FLAG && received[1] == ADDR)
	{
		control = received[2];

		if(control == ll.sequenceNumber)
			return -1;
		
		if(ll.sequenceNumber == 0)
			ll.sequenceNumber = 1;
		else
			ll.sequenceNumber = 0;
			
		// printf("Control: %d\n", control);

		bcc1 = received[3];
		// printf("BCC1: %d\n", bcc1);

		if (bcc1 != received[1] ^ control)
			return -1;
	}

	return control;
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

int readDataPacket2(int *fd, applicationLayer *app, char *buffer, char *filename, char *fileSize, int packetSize)
{	
	int i = 0;
	char controlByte = buffer[i];

	printf("C: %d\n", controlByte);

	if (controlByte == 2) // Start
	{
		checkControlDataPacket2(1, buffer, filename, fileSize, packetSize);

		*fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0777);

	}
	else 
		if (controlByte == 1) // Data
		{
			unsigned char N = buffer[i + 1];
			unsigned char L2 = buffer[i + 2], L1 = buffer[i + 3];

			printf("N: %u\nL1: %u\nL2: %u\n", N, L1, L2);

			if(N != (app->dataPacketIndex - 1) % 255)
			{
				printf("Sequence error\n");
				return -1;
			}

			int K = 256 * L2 + L1;
			printf("K: %d\n", K);

			if(K <= 0)
			{
				printf("Error in packet size\n");
				return -1;
			}
			// else
				//memcpy(buffer, buffer + i + 4, K);

			
			if(write(*fd, buffer+4, K) < 0)
			{
				printf("Error in writting to local file\n");
				return -1;
			}

			printf("--------- Data Packets Read ---------------\n");
			//printArray(buffer, K);

		}
		else 
			if (controlByte == 3) // End
			{
				checkControlDataPacket2(packetSize - 3, buffer, filename, fileSize, packetSize);

				app->status = 2;
			}

	return 0;
}

int checkControlDataPacket2(int i, char *buffer, char *filename, char *fileSize, int packetSize)
{
	int j;
	char T, L;

	for(i = 1; i < packetSize; i += 2 + L)
	{
		T = buffer[i];
		L = buffer[i + 1];

		printf("T: %d\n", T);
		printf("L: %d\n", L);
		printf("i = %d\n", i);

		
		if(T == 0)
		{
			memcpy(fileSize, buffer + i + 2, L);
			printf("File Size: %d\n", *fileSize);
		}	
		else
		{
			if(T == 1)
				memcpy(filename, buffer + i + 2, L);

			printf("File Name: %s\n", filename);
		}
			
	}

	return 0;
}


