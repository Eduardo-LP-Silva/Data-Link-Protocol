/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400 /* bit rate*/
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define BUFFER 255
#define FLAG 0x7E
#define ADDR 0x03
#define SET_C 0x03
#define BCC1 ADDR ^ SET_C
#define UA_C 0x07
#define BCC2 ADDR ^ UA_C
#define ESCAPE 0x7d
#define TRANSMITTER 0
#define RECEIVER 1

volatile int STOP=FALSE;

int timeoutSize = 3;

void sigalrm_handler(int signal)
{
		printf("Message timed out!\n");
		exit(1);
}

int stateMachine(char received[], int C)
{
	int state = 0;
	while (1)
	{
		if (state == 0)
		{
			if (received[0] == FLAG)
				state = 1;
			else
				state = 0;

		//	printf("state = 0 passed\n");
		}
		else if (state == 1)
		{
			if (received[1] == FLAG)
				state = 1;
			else if (received[1] == ADDR)
				state = 2;
			else
				state = 0;
			
			//printf("state = 1 passed\n");
		}
		else if (state == 2)
		{
			if (received[2] == FLAG)
				state = 1;
			else if (received[2] == C)
				state = 3;
			else
				state = 0;

			//printf("state = 2 passed\n");
		}
		else if (state == 3)
		{
			if (received[3] == FLAG)
				state = 1;
			else if (received[3] == ADDR ^ C)
				state = 4;
			else
				state = 0;

		//	printf("state = 3 passed\n");
			//printf("state = %i\n", state);
		}
		else if (state == 4)
		{
			if (received[4] == FLAG)
				return 0;
			else
				state = 0;
		}
	}

	return 1;
} 

int llopen(int fd, int flag)
{
	char setup[5], awns[5];
	int i, received;

	if (flag == TRANSMITTER)
	{
		setup[0] = FLAG;
		setup[1] = ADDR;
		setup[2] = SET_C;
		setup[3] = setup[1] ^ setup[2];
		setup[4] = FLAG;

		if(write(fd, setup, 5) < 0)
		{
			printf("Error in transmission\n");
			return -1;
		}

		printf("Message sent!\n");


		alarm(timeoutSize);
	
		received = read(fd, awns, 5);
	
		alarm(0);

		int j;

		for (j = 0; j < 5; j++)
		{
			printf("Received[%i] = 0x%x\n", j, awns[j]);
		}

		if(received < 0)
		{
			printf("Error in receiving end\n");
			return -1;
		}
	
		int status = stateMachine(awns, UA_C);

		if (!status)
			printf("Received UA\n");
		else
			printf("Unknown message\n");
	}
	else if (flag == RECEIVER)
	{
		alarm(timeoutSize);
	
		received = read(fd, awns, 5);

		alarm(0);
	
		for (i = 0; i < 5; i++)
		{
			printf("Received[%i] = 0x%x\n", i, awns[i]);
		}

		if(received < 0)
		{
			printf("Error in receiving end\n");
			return -1;
		}

		int status = stateMachine(awns, SET_C);

		if (!status)
			printf("Received SET\n");
		else
			printf("Unknown message\n");

		setup[0] = FLAG;
		setup[1] = ADDR;
		setup[2] = UA_C;
		setup[3] = setup[1] ^ setup[2];
		setup[4] = FLAG;

		if(write(fd, setup, 5) < 0)
		{
			printf("Error in transmission\n");
			return -1;
		}

		printf("Message sent!\n");
	}	
	
	return 0;
}

void swap(char* a, char*b)
{
	char temp = *a;
	*a = *b;
	*b = temp;
}

void shiftRight(char* buffer, int size, int position, int shift)
{
	int i, j;
	char temp;
	for (j = 0; j < shift; j++)
	{
		for (i = position; i < size; i++)
		{
			if (i != position)
				swap(&buffer[i], &temp);
			else
				temp = buffer[i];
		}

		position++;
	}
}

int llwrite(int fd, char * buffer, int length)
{
	char package[6 + length + 200], awns[5];
	int i, j, received, packageSize = 6 + length;

	package[0] = FLAG;
	package[1] = ADDR;
	package[2] = 0;
	package[3] = package[1] ^ package[2];
	package[4+length] = buffer[0];

	for (i = 0; i < packageSize; i++)
	{

		package[4+i] = buffer[i];

		if (i != 0)
			package[4+length] = package[4+length] ^ buffer[i];   //BCC2
	}

	package[5+length] = FLAG;

	for (i = 4; i < 4+length; i++)
	{
		if (package[i] == FLAG)
		{
			shiftRight(package, packageSize+1, i+1, 1);
			packageSize++;

			package[i] = ESCAPE;
			package[i+1] = 0x5e;
		}
		else if (package[i] == ESCAPE)
		{
			shiftRight(package, packageSize+1, i+1, 1);
			packageSize++;

			package[i] = ESCAPE;
			package[i+1] = 0x5d;
		}

	}


	for (j = 0; j < packageSize; j++)
	{
		printf("package[%i] = %i\n", j, package[j]);
	}

	if (write(fd, package, packageSize) < 0)
	{
		printf("Error in transmission\n");
		return -1;
	}

	printf("Message sent!\n");


	alarm(timeoutSize);

	received = read(fd, awns, 5);

	alarm(0);

	for (j = 0; j < 5; j++)
	{
		printf("Received[%i] = 0x%x\n", j, awns[j]);
	}

	if(received < 0)
	{
		printf("Error in receiving end\n");
		return -1;
	}

	int status = stateMachine(awns, UA_C);

	if (!status)
		printf("Received UA\n");
	else
		printf("Unknown message\n");

	return 0;
}

void printArray(char* arr)
{
	int i;
	for (i = 0; i < 5; i++)
	{
		printf("%i\n", arr[i]);
	}
}


int main(int argc, char** argv)
{

	char temp[5] = {1,2,FLAG,4,5};

	llwrite(0, temp, 5);

    return 0;
}
