#include "utilities.h"

#include "constants.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

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


void printArray(char* arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		printf("%i\n", arr[i]);
	}
	printf("\n");
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


		alarm(TIMEOUT);
	
		received = read(fd, awns, 5);
	
		alarm(0);
		
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
		alarm(TIMEOUT);
	
		received = read(fd, awns, 5);

		alarm(0);

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