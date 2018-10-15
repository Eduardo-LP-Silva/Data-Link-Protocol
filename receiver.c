#include "receiver.h"
#include "constants.h"
#include "utilities.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int llread(int fd, char * buffer)
{
	char received[128], awns[5];
	int i, j, numBytes = 1, receivedSize;
	
	for (receivedSize = 0; receivedSize < 8; receivedSize++)	// Reads the first 8 bytes of the frame.
	{
		// printf("Before reading\n");
		numBytes = read(fd, &received[receivedSize], 1);
		// printf("Read %i bytes\n", receivedSize);
	}
	printArray(received, receivedSize);

	if (headerCheck(received, receivedSize) != 0)
	{
		printf("Error on checked");
		return -1;
	}
	
	int packageSize = received[6]*256 + received[7];

	for (; receivedSize-8 < packageSize + 2; receivedSize++) // Reads data package plus bcc2 and flag ending the data packet.
	{
		// printf("Before reading\n");
		numBytes = read(fd, &received[receivedSize], 1);
		// printf("Read %i bytes\n", receivedSize);
	}
	printArray(received, receivedSize);

	if (received[receivedSize-1] != FLAG)
	{
		printf("Error retrieving FLAG at the end of the data packet");
		return -1;
	}

	for (i = 8; i < packageSize + 8; i++) // Destuffs the data package
	{
		if (received[i] == ESCAPE)
		{
			if (received[i+1] == 0x5e)
			{
				shiftLeft(received, receivedSize, i+1, 1);
				receivedSize--;

				received[i] = FLAG;
			}
			else if (received[i+1] == 0x5d)
			{
				shiftLeft(received, receivedSize, i+1, 1);
				receivedSize--;

				received[i] = ESCAPE;
			}
			else
			{
				return -1;
			}
			
		}
	}
	packageSize = receivedSize - 6;
	printArray(received, receivedSize);

	//TODO Call data packet check functions

	if (dataCheck(received+4, packageSize+1) != 0)
	{
		printf("Error on the BCC2 component of the data packet");
		return -1;
	}

	for (i = 0; i < packageSize; i++) // Extracts data package to buffer
	{
		buffer[i] = received[8 + i];
	}
	
	sendAnswer(fd, RR_C);

	return 0;
}

int headerCheck(char received[], int size)
{
	char control, bcc1, bcc2;
	int i;

	if (received[0] == FLAG && received[1] == ADDR)
	{
		control = received[2];
		bcc1 = received[3];

		if (bcc1 != received[1] ^ control)
			return -1;

		if (bcc2 != received[size-2])
			return -1;
		else
			return 0;
			
	}

	return -1; //Error
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

int checkDataPacket(char *dataPacket, applicationLayer *app)
{
	char controlByte = dataPacket[0];
	int error = -1;

	switch(controlByte)
	{
		case 1:
			break;

		case 2:
			if((error = checkControlDataPacket(dataPacket)) == -1)
				return error;

			//TODO UPDATE STATE MACHINE - START DATA PACKET TRANSFER

			break;

		case 3:
			if((error = checkControlDataPacket(dataPacket)) == -1)
				return error;

			//TODO UPDATE STATE MACHINE - END DATA PACKET TRANSFER
			
		
			break;

		default:
			return -1;
			break;
	}

	char sequenceNumber = dataPacket[1];

	if(sequenceNumber != app->dataPacketIndex + 1)
	{
		printf("Sequence error\n");
		//TODO Error correction - Send signal to send this frame again (?)
	}
	else
		app->dataPacketIndex++;

	char l1 = dataPacket[2], l2 = dataPacket[3];
	char K = 256 * l2 + l1;
	
	
	//TODO Complete
}

int checkControlDataPacket(char *controlDataPacket)
{
	//TODO Complete
}

