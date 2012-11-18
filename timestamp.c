/* copyright andreas loeffler <al@exitzero.de> 2012 */

#include <stdio.h>
#include <time.h>
#include <string.h>


char* zulutime1(const time_t *timep);
int zulutime(const time_t *timep, char *zulu, size_t len);



char*
zulutime1(const time_t *timep)
{
	static char zulu[22];
	int rc;
	struct tm *tms = gmtime(timep);
	bzero(zulu, 22);

	if (NULL == tms) {
		return NULL;
	}
	rc = snprintf(zulu, 22, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ",
		     tms->tm_year + 1900,
		     tms->tm_mon + 1,
		     tms->tm_mday,
		     tms->tm_hour,
		     tms->tm_min,
		     tms->tm_sec);
	if (rc < 0)
		return NULL;
	return zulu;
}


// len = 22
int
zulutime(const time_t *timep, char *zulu, size_t len)
{
	int rc;
	struct tm *tms;

	bzero(zulu, len);
	tms = gmtime(timep);

	if (NULL == tms) {
		return -1;
	}
	rc = snprintf(zulu, len, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ",
		     tms->tm_year + 1900,
		     tms->tm_mon + 1,
		     tms->tm_mday,
		     tms->tm_hour,
		     tms->tm_min,
		     tms->tm_sec);
	if (rc < 0)
		return -1;
	return 0;
}


int
main(void)
{
	struct tm *tms;
	time_t ltime = time(NULL);
	char timestamp[26];
	char zulu[22];

	bzero(timestamp, 26);
	strncpy(timestamp, ctime(&ltime), 24);
	fprintf(stdout, "%s\thello world\n", timestamp);

	bzero(timestamp, 26);
	strncpy(timestamp, zulutime1(&ltime), 22);
	fprintf(stdout, "%s\thello world\n", timestamp);


	printf("%s\tasctime(localtime())\n", asctime(localtime(&ltime)));
	printf("%s\tasctime(localtime())\n", asctime(gmtime(&ltime)));

	tms = gmtime(&ltime);

	// <time>2009-10-13T13:48:20Z</time>
	printf("zulu time: %.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ\n",
	       tms->tm_year + 1900,
	       tms->tm_mon + 1,
	       tms->tm_mday,
	       tms->tm_hour,
	       tms->tm_min,
	       tms->tm_sec);

	if (-1 == zulutime(&ltime, zulu, 22)) {
		fprintf(stderr, "zulutime error\n");
	}
	printf("zulutime: %s\n", zulu);

	return 0;
}
