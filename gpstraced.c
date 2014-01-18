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

const char *logfile = "/var/log/gpstraced.log";
const char *gpstracefile = "/tmp/gpstrace.txt";
const char const *version = "gpstraced version 0.2";

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

/**
 * open log/trace file for writing/appending. file must exist!
 */
FILE *open_logfile(const char *);

/**
 * log error msg and exit
 */
void log_error(const char* msg, int error, FILE* log);

/**
 * log info msg
 */
	int log_info(const char* msg, FILE* log);

/**
 * open/create new gpx xml file and add header data.
 */
FILE *open_newxml(void);

/**
 * write gpx "footer" and close file.
 */
int close_xmlfile(FILE *);

/**
 * parse incoming msg from gps tracker and put into gpx file.
 */
int parse_and_write(const char *, FILE *, FILE *);

/**
 * degmin2dec
 * convert the input lat or lon coordinate given as a string in degree and
 * decimal minute into a decimal (degree) number.
 */
float degmin2dec(const char *degmin, const char *dir);


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

	sprintf(xmlfile, "/tmp/gpstraced_%s.gpx", timestamp);

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

// FIXME: the "trace" file is obsolete now since we log to a gpx file
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

	dec_lat = degmin2dec(store[5], store[6]);
	dec_lon = degmin2dec(store[7], store[8]);

	fprintf(trace, "%s, %s, %s, %s, %f, %f, %s, %s\n", timestamp, store[3],
		store[5], store[7], dec_lat, dec_lon, store[16], store[17]);

	/* write xml */
	fprintf(xml, "<trkpt lat=\"%f\" lon=\"%f\">\n",
		dec_lat, dec_lon);
	fprintf(xml, " <ele>%s</ele>\n", store[16]); // GPS provided elevation
	fprintf(xml, " <time>%s</time>\n", timestamp);
	fprintf(xml, "</trkpt>\n");

	free(store);
	free(s);

	fflush(trace);
	fflush(xml);

	return 0;
}

// FIXME: direction north/south, east/west
float
degmin2dec(const char *degmin, const char *dir)
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
			return -1.0;
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
			return -1.0;
		}
		deg_min = div(lat1, 100);
		ret = deg_min.quot + (lat2 / 10000.0 + deg_min.rem) / 60;
		return ret;
	}
	return ret;
}


int
main(int argc __attribute__((__unused__)),
     char *argv[] __attribute__((__unused__)))
{
	pid_t pid;
	FILE *log;
	FILE *trace;
	FILE *xml;
	unsigned int listen_port = 0;
	int c;
	char tracker_id[32] = "";

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"port", 1, 0, 'p'},
			{"trackerid", 1, 0, 'i'},
			{"verbose", 0, 0, 0},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "p:i:v",
				long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'p':
			listen_port = strtol(optarg, (char **) NULL, 10);
			if ((listen_port < 32768) || (listen_port > 65535)) {
				fprintf(stderr, "Invalid value for port (only 32768 to 65535)");
				exit(EXIT_FAILURE);
			}
			break;
		case 'i':
			snprintf(tracker_id, 16, "STX,%s", optarg);
			//printf("tracker id with value '%s'\n", tracker_id);
			break;
			//default:
			//	break;
		}
	}
	if (0 == listen_port && 0 == strlen(tracker_id)) {
		fprintf(stderr, "Usage: gpstraced --port|-p PORT --trackerid|-i TRACKERID\n");
		exit(EXIT_FAILURE);
	}

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
	serv_addr.sin_port = htons(listen_port);

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
			//log_info(tracker_id, log);
			if (0 == strncmp(tracker_id, buff, 10)) {
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
