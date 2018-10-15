#include "transmitter.h"

#include "constants.h"
#include "utilities.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int sendFile(char* filename, char* device)
{
	int portfd = openPort(device, TRANSMITTER);
	
	int fd = open(filename, O_RDONLY);

	struct stat st;

	if (stat(filename, &st) != 0)
	{
		printf("Failed to get file size!\n");
		return 1;
	}

	int size = st.st_size;
	
	char buffer[size], package[300];
	int packageSize = 0;

	// Constructs start control package

	package[packageSize++] = 2; // C (2 - start)
	package[packageSize++] = 0; // field type (file size)
	package[packageSize++] = 1; // Number of bytes of field
	package[packageSize++] = size;

	package[packageSize++] = 1; // field type (file size)
	package[packageSize++] = strlen(filename)+1; // Number of bytes of field
	memcpy(&package[packageSize], filename, strlen(filename) + 1);
	packageSize += strlen(filename) + 1;

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
	
	for (i = 0; i < size/128 + 1; i++)
	{
		numBytes = (size  - 128*i)%128;

		package[packageSize++] = 1; // C (1 - data) 
		package[packageSize++] = i; // Sequence number
		package[packageSize++] = numBytes / 256; // The 8 most significant bits in the packageSize.
		package[packageSize++] = numBytes % 256;
		memcpy(&package[packageSize], buffer+i*128, numBytes);
		packageSize += numBytes;

		if (numBytes < 128) // Constructs end control package
		{
			package[packageSize++] = 3; // C (3 - end)
			package[packageSize++] = 0; // field type (file size)
			package[packageSize++] = 1; // Number of bytes of field
			package[packageSize++] = numBytes;

			package[packageSize++] = 1; // field type (file size)
			package[packageSize++] = strlen(filename)+1; // Number of bytes of field
			memcpy(&package[packageSize], filename, strlen(filename) + 1);
			packageSize += strlen(filename) + 1;
		}

		llwrite(portfd, package, packageSize);

		char received[5];

		alarm(TIMEOUT);
		
		read(fd, received, 5);

		alarm(0);

		int control = messageCheck(received);

		if (control == REJ_C)
		{
			printf("Corrupt package sent, sending same package again!\n");
			i--;
		}
		else if (control == RR_C)
		{
			printf("Package sent sucessfully\n");
		}
		else
		{
			printf("Unknown answer\n");
		}

		packageSize = 0;
	}

	close(fd);

	llclose(portfd);

	return 0;
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
