/*Non-Canonical Input Processing*/

#include "main.h"

volatile int STOP=FALSE;

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


int messageCheck(char received[])
{
	char control, bcc1, bcc2;
	int i;

	if (received[0] == FLAG && received[1] == ADDR && received[4] == FLAG)
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
	
	alarm(timeoutSize);
	
	read(fd, received, 5);

	alarm(0);	

	return written;
}

int llread(int fd, char * buffer)
{
	char received[128], awns[5];
	int i, j, numBytes = 1, receivedSize;
	
	for (receivedSize = 0; receivedSize < 8; receivedSize++)	// Reads the first 8 bytes of the data packet.
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
	packageSize = receivedSize - 8;
	printArray(received, receivedSize);

	if (dataCheck(received+4, receivedSize-4) != 0)
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


int llclose(int fd)
{
	

	return 1;
}


int main(int argc, char** argv)
{
	int fd,c, res;
	struct termios oldtio,newtio;
	char buf[255];

	if ( (argc < 2) || 
  		 ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  		  (strcmp("/dev/ttyS1", argv[1])!=0) )) {
	  printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
	  exit(1);
	}


  /*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
  */
  
	
	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0) {perror(argv[1]); exit(-1); }

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
	newtio.c_cc[VMIN]	 = 1;   /* blocking read until 1 chars received */



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
	
	if (strcmp(argv[2], "transmitter") == 0)
	{
		llopen(fd, TRANSMITTER);
	
		int size = 5;
		char temp[5] = {1, 2, FLAG, 4, 5};

		printf("Return : %i\n", llwrite(fd, temp, size));
	
	}
	else if (strcmp(argv[2], "receiver") == 0)
	{
		llopen(fd, RECEIVER);
	
		char received[100];

		printf("Return : %i\n", llread(fd, received));
	}
	else
	{
		printf("Must specify \"transmitter\" or \"receiver\" as second argument\n");
		return -1;
	}
	
	
	

    tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
	return 0;
}
