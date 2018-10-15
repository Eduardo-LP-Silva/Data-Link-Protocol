#include "transmitter.h"

#include "constants.h"
#include "utilities.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int llwrite(int fd, char * buffer, int length)
{
	char package[6 + length + 200], awns[5];
	int i, j, packageSize = 10 + length;

	package[0] = FLAG;
	package[1] = ADDR;
	package[2] = 0;
	package[3] = package[1] ^ package[2];
	package[4] = 1; /* 1 - dados ??*/
	package[5] = 0 % 255;
	
	int packageLength = length;
	
	for (i = 0; i < length; i++) // Measures size of data
	{
		if (package[i] == FLAG || package[i] == ESCAPE)
			packageLength++;
	}
	
	package[6] = packageLength / 256;
	package[7] = packageLength % 256;
	
	for (i = 4; i < 8; i++)								// Calculates BCC2
	{
		if (i == 4)
			package[packageSize-2] = package[i];
		else
			package[packageSize-2] ^= package[i];
	}
	
	for (i = 0; i < length; i++)
	{
		package[8+i] = buffer[i];
		
		package[packageSize-2] ^= buffer[i];
	}

	for (i = 8; i < packageSize - 2; i++) // Stuffing
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