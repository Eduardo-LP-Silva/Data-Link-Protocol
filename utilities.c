#include "utilities.h"

#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
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

int openPort(char* device, int flag)
{
	int fd = open(device, O_RDWR | O_NOCTTY);

	if (fd < 0)
	{
		perror(device);
		exit(-1);
	}
	
	return llopen(fd, flag);
}

int llopen(int fd, int flag)
{
	int c, res;

  /*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
  */

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
	  perror("tcgetattr");
	  exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]	= 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]	= 1;   /* blocking read until 1 chars received */



  /* 
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
	leitura do(s) próximo(s) caracter(es)
  */


	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
	  perror("tcsetattr");
	  exit(-1);
	}

	printf("New termios structure set\n");

	char buf[5];
	int received;

	if (flag == TRANSMITTER)
	{
		buf[0] = FLAG;
		buf[1] = ADDR;
		buf[2] = SET_C;
		buf[3] = buf[1] ^ buf[2];
		buf[4] = FLAG;

		if(write(fd, buf, 5) < 0)
		{
			printf("Error in transmission\n");
			return -1;
		}

		printf("Message sent!\n");


		alarm(TIMEOUT);
	
		received = read(fd, buf, 5);
	
		alarm(0);
		
		if(received < 0)
		{
			printf("Error in receiving end\n");
			return -1;
		}
	
		int status = stateMachine(buf, UA_C);

		if (!status)
			printf("Received UA\n");
		else
			printf("Unknown message\n");
	}
	else if (flag == RECEIVER)
	{
		alarm(TIMEOUT);
	
		received = read(fd, buf, 5);

		alarm(0);

		if(received < 0)
		{
			printf("Error in receiving end\n");
			return -1;
		}

		int status = stateMachine(buf, SET_C);

		if (!status)
			printf("Received SET\n");
		else
			printf("Unknown message\n");

		buf[0] = FLAG;
		buf[1] = ADDR;
		buf[2] = UA_C;
		buf[3] = buf[1] ^ buf[2];
		buf[4] = FLAG;

		if(write(fd, buf, 5) < 0)
		{
			printf("Error in transmission\n");
			return -1;
		}

		printf("Message sent!\n");
	}	
	
	return fd;
}


int llclose(int fd)
{
	tcsetattr(fd,TCSANOW,&oldtio); 
	close(fd);
}