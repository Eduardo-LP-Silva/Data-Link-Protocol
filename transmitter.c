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
	
	char* buffer = malloc(128);
	char* package = malloc(300);
	int packageSize = 0, numBytes = 128;

	// Control package

	package[packageSize++] = 2; // C (2 - start)
	package[packageSize++] = 0; // field type (file size)
	package[packageSize++] = 1; // Number of bytes of field
	package[packageSize++] = numBytes;

	package[packageSize++] = 1; // field type (file size)
	package[packageSize++] = strlen(filename)+1; // Number of bytes of field
	memcpy(&package[packageSize], filename, strlen(filename) + 1);
	packageSize += strlen(filename) + 1;
	
	int i;
	for (i = 0; numBytes == 128; i++)
	{
		numBytes = read(fd, buffer, 128);
		
		package[packageSize++] = 1; // C (1 - data) 
		package[packageSize++] = i; // Sequence number
		package[packageSize++] = numBytes / 256; // The 8 most significant bits in the packageSize.
		package[packageSize++] = numBytes % 256;
		memcpy(&package[packageSize], buffer, numBytes);
		packageSize += numBytes;

		llwrite(fd, package, packageSize);

		packageSize = 0;
	}

	llclose(portfd);

	return 0;
}


int llwrite(int fd, char * buffer, int length)
{
	char package[6 + length + 200], awns[5];
	int i, j, packageSize = 10 + length;

	package[0] = FLAG;
	package[1] = ADDR;
	package[2] = 0;
	package[3] = package[1] ^ package[2];
	
	int packageLength = length;
	
	for (i = 0; i < length; i++) // Measures size of data
	{
		if (package[i] == FLAG || package[i] == ESCAPE)
			packageLength++;
	}
	
	for (i = 0; i < length; i++) // Calculates BCC2
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
		
	char received[5];
	
	alarm(TIMEOUT);
	
	read(fd, received, 5);

	alarm(0);	

	return written;
}
