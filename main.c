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

void printArray(char* arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		printf("%i\n", arr[i]);
	}
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

int dataCheck(char received[], int size, char *package)
{
	char control, bcc1, bcc2;
	int i;

	if (received[0] == FLAG && received[1] == ADDR && received[size-1] == FLAG)
	{
		control = received[2];
		bcc1 = received[4];

		if (bcc1 != received[1] ^ control)
			return -1;

		
		for (i = 4; i < size-1; i++)
		{
			package[i-4] = received[i];

			if (i == 4)
				bcc2 = received[4];
			else
				bcc2 = bcc2 ^ received[i];
		}
	}

	return -1; //Error
}


int messageCheck(char received[])
{
	char control, bcc1, bcc2;
	int i;

	if (received[0] == FLAG && received[1] == ADDR && received[size-1] == FLAG)
	{
		control = received[2];
		bcc1 = received[4];

		if (bcc1 == received[1] ^ control)
			return control;
	}

	return -1; //Error
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

int abs(int a)
{
	if (a < 0)
		return -a;
	return a;
}

void shiftRight(char* buffer, int size, int position, int shift)
{
	int i, j;

	for (j = 0; j < shift; j++)
	{
		size++;
		buffer[size-1] = 0;

		for (i = size-2; i >= position; i--)
		{
			swap(&buffer[i], &buffer[i+1]);
		}

		position++;
	}
}

void shiftLeft(char* buffer, int size, int position, int shift)
{
	int i, j;

	for (j = 0; j < shift; j++)
	{

		for (i = position-1; i < size; i++)
		{
			swap(&buffer[i], &buffer[i+1]);
		}

		size--;
		position--;
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

	printArray(package, packageSize);

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


	printArray(package, packageSize);

	if (write(fd, package, packageSize) < 0)
	{
		printf("Error in transmission\n");
		return -1;
	}

	printf("Message sent!\n");

	alarm(timeoutSize);

	received = read(fd, awns, 5);

	alarm(0);

	printArray(awns, 5);

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

int llread(int fd, char * buffer)
{
	int receivedSize = 128;
	char received[receivedSize], awns[5];
	int i, j, numBytes = 1;

	for (i = 0; numBytes < 1; i++)
	{
		numBytes = read(fd, received+i, 1);
	}

	int bytesReceived = i;
	printArray(received, bytesReceived);

	/*if (package[i] == FLAG)
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
	}*/



	if (write(fd, package, packageSize) < 0)
	{
		printf("Error in transmission\n");
		return -1;
	}

	printf("Message sent!\n");

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


int main(int argc, char** argv)
{

	int size = 7;
	char temp[7] = {1, 2, 0, 0, 4, 5, 6};
	int position = 4;
	int shiftValue = 2;

	// llwrite(0, temp, 5);
	
	printArray(temp, size);
	printf("\n");

	shiftLeft(temp, size, position, shiftValue);
	size -= shiftValue;
/*	temp[position] = 69;
	temp[position+1] = -1;
*/
	printArray(temp, size);

    return 0;
}
