#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

double getsecond(char* line)
{
	int i;
	for (i = 0; i < strlen(line); i++)
	{
		if (line[i] == ',')
		{
			double res;
			sscanf(line+i+2, "%lf", &res);
			return res;
		}

	}


return 0;

}
                             


int main()
{
	int fd = open("log2", O_RDONLY);

	int numBytes = 1;
	int i;
	double sum = 0;
	char line[1000];
	int counter = 0;
	for (i = 0; numBytes > 0; i++)
	{
		numBytes = read(fd, &line[i], 1);

		if (line[i] == '\n')
		{
			line[i] = 0;
			sum += getsecond(line);
			i = 0;
			counter++;
		}
		
	}

	printf("Sum = %f\n", sum/counter);	

	return 0;
}
