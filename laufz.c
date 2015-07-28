
#define _ISOC99_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <string.h>




int main(int argc, char *argv[])
{
	int   hour,min, sec;
	int   total_sec;
	div_t tkm;
	float t, km, speed;
	char *tok;
	const char delimiters[] = " :";


	if(argc < 3) {
		fprintf(stderr, "Usage: %s, km(eg. 21.1) time(hh:mm:ss)\n",
			argv[0]);
		exit(EXIT_FAILURE);
	}

	km = strtof(argv[1], NULL);

	printf("%s: complied on "__DATE__" "__TIME__"\n\n", argv[0]);

	/* FIXME: check for errors .. if not n:n:n  */
//	strtol(nptr, (char **)NULL, 10);
	tok = strtok (argv[2], delimiters);
	if (NULL != tok)
		hour = atoi(tok);
	tok = strtok (NULL, delimiters);
	if (NULL != tok)
		min = atoi(tok);
	tok = strtok (NULL, delimiters);
	if (NULL != tok)
		sec = atoi(tok);

	total_sec = sec + (60 * min) + (60 * 60 * hour);
//	printf("km=%.3f, time: hour=%d, min=%d, sec=%d\n", km, hour, min, sec);
//	printf("total_sec=%d\n", total_sec);

	t = total_sec/km;
//	printf("t=%f\n", t);

	speed = (km / total_sec) * 3600;


	printf("Speed = %.2f km/h\n", speed);

	tkm = div (t, 60);

	printf("Pace  = %d min %d sec\n", tkm.quot, tkm.rem);

	exit(EXIT_SUCCESS);
}

