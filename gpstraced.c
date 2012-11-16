/**
 * gpstraced.c - a network daemon listening on tcp socket for connections
 *               from a gsp/gsm tracking device. in this case a CatTrak Live 3
 *               write messages to log file and generate a gpx file.
 *
 * Copyright (c) by Andreas Loeffler <al@exitzero.de>  2012
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#define __USE_GNU
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <getopt.h>
#include <argp.h>

#include "gpstraced.h"

#define MAX_SIZE 256
#define LISTEN_PORT 1337

const char *logfile = "/var/log/gpstraced.log";
const char *gpstracefile = "/tmp/gpstrace.txt";

/**
 * convert string degree/decimal minutes to decimal degree floating point num
 */
float deg2dec(const char *, const char *);
/**
 * Split string at delimiter. Caller need to free returned pointer of pointers.
 */
char** splitstr(char* in_str, const char* delims, int *count);
/**
 * Return zulu time as string.
 */
char* zulutime(const time_t *);


char*
zulutime(const time_t *timep)
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

FILE *
open_newxml(void)
{
	char xmlfile[128];
	time_t ltime = time(NULL);
	char timestamp[22];
	int fd;
	FILE *xml;

	bzero(timestamp, 22);
	strncpy(timestamp, zulutime(&ltime), 22);

	sprintf(xmlfile, "/tmp/gpstraced_%s.xml", timestamp);

	fd = open(xmlfile, O_WRONLY|O_APPEND|O_NOFOLLOW|O_NONBLOCK|O_CREAT,
		  S_IRUSR|S_IRGRP|S_IROTH);
	if (-1 == fd) {
		fprintf(stderr, "xmlfile: %s. ", xmlfile);
		perror("opening logfile");
		return NULL;
	}
	xml = fdopen(fd, "a");
	if (NULL == xml) {
		perror("fdopen logfile");
		return NULL;
	}
	fprintf(xml, "%s%s%s\n", xml_head_1, timestamp, xml_head_2);
	fflush(xml);

	return xml;
}

int
close_xmlfile(FILE *xml)
{
	fprintf(xml, "%s\n", xml_end);
	fflush(xml);
	if (fclose(xml))
		return -1;
	return 0;
}

/**
 * open log/trace file for writing/appending. file must exist
 */
FILE *
open_logfile(const char * filename)
{
	int logfd;
	FILE *log;
	logfd = open(filename, O_WRONLY|O_APPEND|O_NOFOLLOW|O_NONBLOCK);
	if (-1 == logfd) {
		fprintf(stderr, "logfile: %s. ", filename);
		perror("opening logfile");
		return NULL;
	}
	log = fdopen(logfd, "a");
	if (NULL == log) {
		perror("fdopen logfile");
		return NULL;
	}
	return log;
}

/**
 * log error msg and exit
 */
void
log_error(const char* msg, int error, FILE* log)
{
	time_t ltime = time(NULL);
	char timestamp[26];

	bzero(timestamp, 26);
	strncpy(timestamp, ctime(&ltime), 24);
	fprintf(log, "%s\t%s : %s\n", timestamp, msg, strerror(error));
	fflush(log);

	exit(EXIT_FAILURE);
}

/**
 * log info msg
 */
int
log_info(const char* msg, FILE* log)
{
	time_t ltime = time(NULL);
	char timestamp[26];

	bzero(timestamp, 26);
	strncpy(timestamp, ctime(&ltime), 24);
	fprintf(log, "%s\t%s\n", timestamp, msg);
	fflush(log);

	return 0;
}

/**
 * split a string into pieces, output to array of char pointer
 * -> caller must use free!!!
 * count will get the number of elements of the array.
 */
char **
splitstr(char* in_str, const char* delims, int *count)
{
	char *result = NULL;
	char **store = NULL;
	char **tmp = NULL;
	int i;
	*count = 0;

	result = strtok(in_str, delims);
	if (result != NULL) {
		store = malloc(((*count) + 1) * sizeof(char *));
		store[*count] = result;
		(*count)++;
		tmp = malloc((*count) * sizeof(char *));
		result = strtok(NULL, delims);
	}
	while (result != NULL) {
		free(tmp);
		tmp = malloc((*count) * sizeof(char *));
		for (i=0; i<(*count); i++) {
			tmp[i] = store[i];
		}
		free(store);
		store = malloc(((*count) + 1) * sizeof(char *));
		for (i=0; i<(*count); i++) {
			store[i] = tmp[i];
		}
		store[*count] = result;
		(*count)++;

		result = strtok(NULL, delims);
	}
	free(tmp);
	return store;
}

// STX,CAT001,$GPRMC,192047.000,A,4840.5580,N,00859.8861,E,0.22,355.29,101112,,,A*6E,F,,imei:012896005317034,05,437.2,Battery=75%,,1,262,03,22D7,BDD7;9F
//    RMC          Recommended Minimum sentence C
//    123519       Fix taken at 12:35:19 UTC
//    A            Status A=active or V=Void.
//    4807.038,N   Latitude 48 deg 07.038' N
//    01131.000,E  Longitude 11 deg 31.000' E
//    022.4        Speed over the ground in knots
//    084.4        Track angle in degrees True
//    230394       Date - 23rd of March 1994
//    003.1,W      Magnetic Variation
//    *6A          The checksum data, always begins with *
/*
 * split done, count=23
 * array[0]  is STX
 * array[1]  is CAT001
 * array[2]  is $GPRMC
 * array[3]  is 192047.000
 * array[4]  is A
 * array[5]  is 4840.5580
 * array[6]  is N
 * array[7]  is 00859.8861
 * array[8]  is E
 * array[9]  is 0.22
 * array[10] is 355.29
 * array[11] is 101112
 * array[12] is A*6E
 * array[13] is F
 * array[14] is imei:012896005317034
 * array[15] is 05
 * array[16] is 437.2
 * array[17] is Battery=75%
 * array[18] is 1
 * array[19] is 262
 * array[20] is 03
 * array[21] is 22D7
 * array[22] is BDD7;9F
 */

int
parse_and_write(const char* msg, FILE* trace, FILE* xml)
{
	time_t ltime = time(NULL);
	char timestamp[26];

	int count = 0;
	//int i;
	char **store = NULL;
	char *s;
	float dec_lat, dec_lon;

	bzero(timestamp, 26);
	strncpy(timestamp, zulutime(&ltime), 24);

	s = malloc(strlen(msg)+1);
	memcpy(s, msg, strlen(msg)+1);

	store = splitstr(s, ",", &count);

	dec_lat = deg2dec(store[5], store[6]);
	dec_lon = deg2dec(store[7], store[8]);

	// 2012-11-16T14:14:35Z, 192047.000, 4840.5580, 00859.8861, 48.675968, 8.998101, Battery=75%
	// timestamp           , store[3]  , store[5],  store[7],   dec_lat  , dec_lon , store[17]
	fprintf(trace, "%s, %s, %s, %s, %f, %f, %s, %s\n", timestamp, store[3],
		store[5], store[7], dec_lat, dec_lon, store[16], store[17]);

	/* write xml */
	fprintf(xml, "<trkpt lat=\"%f\" lon=\"%f\"></trkpt>\n",
		dec_lat, dec_lon);
	fprintf(xml, " <ele>%s</ele>\n", store[16]); // GPS provided elevation
	fprintf(xml, " <time>%s</time>\n", timestamp);
	fprintf(xml, "</trkpt>\n");

	/* myLieu xml geo file ...
	 * <?xml version="1.0" encoding="iso-8859-1"?>
	 * <markers><marker time="2009-02-15T19:32:33.000Z+1:00" lat="49.302" lng="8.69" speed="12" alt="110" dir="0.0"/></markers>
	 */

	free(store);
	free(s);

	fflush(trace);
	fflush(xml);

	return 0;
}

// FIXME: direction north/south, east/west
float
deg2dec(const char *degmin, const char *dir)
{
	int lat1, lat2;
	int n;
	float ret = -1.0;
	div_t deg_min;

	if (0 == strcmp(dir, "N")) {
		errno = 0;
		n = sscanf(degmin, "%d.%d", &lat1, &lat2);
		if (2 != n) {
			perror("sscanf failed");
			return -1;
		}
		deg_min = div(lat1, 100);
		ret = deg_min.quot + (lat2 / 10000.0 + deg_min.rem) / 60;
		return ret;
	}

	if (0 == strcmp(dir, "E")) {
		errno = 0;
		n = sscanf(degmin, "%d.%d", &lat1, &lat2);
		if (2 != n) {
			perror("sscanf failed");
			return -1;
		}
		deg_min = div(lat1, 100);
		ret = deg_min.quot + (lat2 / 10000.0 + deg_min.rem) / 60;
		return ret;
	}
		return ret;
}


int
main(int argc, char *argv[])
{
	pid_t pid;
	FILE *log;
	FILE *trace;
	FILE *xml;

	// FIXME: cmdline args for port, filenames, ...

	log = open_logfile(logfile);
	if (NULL == log) {
		exit(EXIT_FAILURE);
	}
	trace = open_logfile(gpstracefile);
	if (NULL == trace) {
		exit(EXIT_FAILURE);
	}
	xml = open_newxml();
	if (NULL == log) {
		exit(EXIT_FAILURE);
	}


	fprintf(stderr, "gpstraced started\n");
	if (0 != chdir("/")) {
		perror("chdir('/')");
		exit(1);
	}
	if ((pid = fork()) < 0) {
		perror("fork");
		exit(1);
	}
	/* exit parent */
	if (pid != 0)
		exit(0);
	/* child continues */
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	setuid(65534); /* my name is nobody ... */
	setgid(65534);
	setsid();


	// fixme: consider putting the bind call before forking the child...
	int sock_descriptor, conn_desc;
	struct sockaddr_in serv_addr;
	char buff[MAX_SIZE];

	log_info("*** gpstraced started ***", log);

	sock_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_descriptor < 0) {
		log_error("Failed creating socket", errno, log);
	}

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(LISTEN_PORT);

	if (bind(sock_descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		log_error("Failed to bind", errno, log);
	}

	if (listen(sock_descriptor, 5) < 0) {
		log_error("Failed to listen", errno, log);
	}


	/* waiting for tracker to connect */
	while (1) {
		/*
		 * server blocks on this call until a client tries to establish
		 * connection. once connected a new connected socket descriptor
		 * is returned
		 */
		conn_desc = accept(sock_descriptor, NULL, NULL);
		if (conn_desc == -1) {
			log_info("Failed accepting connection ... EXITING", log);
			close(sock_descriptor);
			exit(EXIT_FAILURE);
		}

		/* get and log client ip address */
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		char client_ip[16];
		char connect_msg[32];
		if (-1 == getsockname(conn_desc, (struct sockaddr *)&client_addr,
				      &client_len)) {
			log_error("Failed getting client address ... EXITING", errno, log);

		}
		if (NULL == inet_ntop(AF_INET, &client_addr.sin_addr,
				      client_ip, sizeof(client_ip))) {
			log_error("inet_ntop failed", errno, log);
		}
		snprintf(connect_msg, 32, "connection from %s", client_ip);
		log_info(connect_msg, log);


		bzero(buff, sizeof(buff));
		if (read(conn_desc, buff, sizeof(buff)-1) > 0) {
			log_info(buff, log);
			if (0 == strncmp("EXIT;", buff, 5)) {
				log_info("Good Bye\n", log);
				close(conn_desc);
				close(sock_descriptor);
				close_xmlfile(xml);
				exit(EXIT_SUCCESS);
			}
			// catTraq specific!!
			if (0 == strncmp("STX,", buff, 4)) {
				parse_and_write(buff, trace, xml);
			}

		}
		else {
			log_info("Failed receiving", log);
		}
		if (close(conn_desc) < 0) {
			log_error("Failed to close connection socket", errno, log);
		}
	}

	exit(EXIT_FAILURE);
}