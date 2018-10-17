#include "transmitter.h"

#include "constants.h"
#include "utilities.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sigalrm_handler(int signal)
{
	printf("Message timed out!\n");
	
}


int stateMachine(char* device, char* buffer, int size, char* filename)
{
	applicationLayer al;
	al.status = 0;
	al.flag = TRANSMITTER;
	al.dataPacketIndex = 0;

	int packageSize = 0, numBytes;

	char** tramaArray = calloc((size/128 + 1) + 2, sizeof(char));

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
		}
		else if (al.status == 1) // Transfering
		{
			if (tramaArray[al.dataPacketIndex] == NULL)
			{
				packageSize = 0;

				tramaArray[al.dataPacketIndex] = malloc(128 + 6 + 1);

				if (al.dataPacketIndex == 0) // Start
				{
					tramaArray[al.dataPacketIndex][packageSize++] = 2; // C (2 - start)
					tramaArray[al.dataPacketIndex][packageSize++] = 0; // field type (file size)
					tramaArray[al.dataPacketIndex][packageSize++] = 1; // Number of bytes of field
					tramaArray[al.dataPacketIndex][packageSize++] = size;

					tramaArray[al.dataPacketIndex][packageSize++] = 1; // field type (file size)
					tramaArray[al.dataPacketIndex][packageSize++] = strlen(filename)+1; // Number of bytes of field
					memcpy(&tramaArray[al.dataPacketIndex][packageSize], filename, strlen(filename) + 1);
					packageSize += strlen(filename) + 1;
				}
				else if (al.dataPacketIndex == (size/128 + 1)) // Data Packages
				{
					numBytes = (size  - 128*al.dataPacketIndex)%128;

					tramaArray[al.dataPacketIndex][packageSize++] = 1; // C (1 - data) 
					tramaArray[al.dataPacketIndex][packageSize++] = al.dataPacketIndex; // Sequence number
					tramaArray[al.dataPacketIndex][packageSize++] = numBytes / 256; // The 8 most significant bits in the packageSize.
					tramaArray[al.dataPacketIndex][packageSize++] = numBytes % 256;
					memcpy(&tramaArray[al.dataPacketIndex][packageSize], buffer+al.dataPacketIndex*128, numBytes);
					packageSize += numBytes;
				}
				else // End
				{
					tramaArray[al.dataPacketIndex][packageSize++] = 3; // C (3 - end)
					tramaArray[al.dataPacketIndex][packageSize++] = 0; // field type (file size)
					tramaArray[al.dataPacketIndex][packageSize++] = 1; // Number of bytes of field
					tramaArray[al.dataPacketIndex][packageSize++] = numBytes;

					tramaArray[al.dataPacketIndex][packageSize++] = 1; // field type (file size)
					tramaArray[al.dataPacketIndex][packageSize++] = strlen(filename)+1; // Number of bytes of field
					memcpy(&tramaArray[al.dataPacketIndex][packageSize], filename, strlen(filename) + 1);
					packageSize += strlen(filename) + 1;
				}

				tramaArray[al.dataPacketIndex][0] = packageSize;
			}

			llwrite(al.fileDescriptor, tramaArray[al.dataPacketIndex]+1, tramaArray[al.dataPacketIndex][0]);

			char received[5];

			alarm(TIMEOUT);
			
			read(al.fileDescriptor, received, 5);

			alarm(0);

			int control = messageCheck(received);

			if (control == REJ_C)
			{
				printf("Corrupt package sent, sending same package again!\n");
				al.dataPacketIndex--;
			}
			else if (control == RR_C)
			{
				printf("Package sent sucessfully\n");
			}
			else
			{
				printf("Unknown answer\n");
			}

			al.dataPacketIndex++;

			if (al.dataPacketIndex > (size/128 + 1))
				al.status = 2;
		}
		else if (al.status == 2) // Closing
		{
			
			break;
		}
	}
}

int sendFile(char* filename, char* device)
{	
	int fd = open(filename, O_RDONLY);

	struct stat st;

	if (stat(filename, &st) != 0)
	{
		printf("Failed to get file size!\n");
		return 1;
	}

	int size = st.st_size;
	
	char buffer[size];

	int i, numBytes = 128;
	for (i = 0; numBytes == 128; i++)
	{
		numBytes = read(fd, buffer+i*1024, 1024);

		if (numBytes < 0)
		{
			printf("Error reading file!\n");
			return 1;
		}
	}
	close(fd);

	return stateMachine(device, buffer, size, filename);
}


int llwrite(int fd, char * buffer, int length)
{
	char package[6 + 2*length], awns[5];
	int i, j, packageSize = 6+length;

	package[0] = FLAG;
	package[1] = ADDR;
	package[2] = 0;
	package[3] = package[1] ^ package[2];
	
	int packageLength = length;
	
	for (i = 0; i < length; i++) // Measures size of data
	{
		if (buffer[i] == FLAG || buffer[i] == ESCAPE)
			packageLength++;
	}
	
	for (i = 0; i < length; i++) // Transfers data from buffer to package and  calculates BCC2
	{
		package[4+i] = buffer[i];
		
		if (i == 0)
			package[packageSize-2] = buffer[i];
		else
			package[packageSize-2] ^= buffer[i];
	}

	for (i = 4; i < packageSize - 2; i++) // Stuffing
	{
		if (package[i] == FLAG)
		{
			shiftRight(package, packageSize+1, i+1, 1);
			packageSize++;

			package[i] = ESCAPE;
			package[i+1] = 0x5e;
			i++;
		}
		else if (package[i] == ESCAPE)
		{
			shiftRight(package, packageSize+1, i+1, 1);
			packageSize++;

			package[i] = ESCAPE;
			package[i+1] = 0x5d;
			i++;
		}

	}
	
	package[packageSize-1] = FLAG;

	printArray(package, packageSize);

	int written = write(fd, package, packageSize);
	
	if (written < 0)
	{
		printf("Error in transmission\n");
		return -1;
	}
	
	printf("Message sent!\n");

	return written;
}
