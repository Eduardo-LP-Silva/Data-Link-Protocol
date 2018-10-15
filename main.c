#include "main.h"

#include "constants.h"
#include "transmitter.h"
#include "receiver.h"
#include "utilities.h"

void sigalrm_handler(int signal)
{
		printf("Message timed out!\n");
		exit(1);
}


int sendFile(char* filename)
{

	return 0;
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
