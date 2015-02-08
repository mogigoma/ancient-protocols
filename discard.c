/******************************************************************************
 * Copyright (c) 2009 Matthew Anthony Kolybabi (Mak)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ******************************************************************************/

#include <netinet/in.h>

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BACKLOG		5
#define BUF_SIZE	1024
#define PORT_NUM	9

static void *
client_handler(void *data)
{
	struct sockaddr_in from;
	char buf[BUF_SIZE];
	int bytes, fd;
	socklen_t len;

	// Sanity check.
	assert(data != NULL);

	// Perform discard on socket.
	fd = *((int *) data);
	while (true)
	{
		// Read packet.
		len = sizeof(from);
		bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &from, &len);
		if (bytes <= 0)
			break;
	}
	close(fd);

	return (NULL);
}

static int
create_socket(int type)
{
	struct sockaddr_in server;
	int fd, rc;

	// Create socket.
	fd = socket(AF_INET, type, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "socket");

	// Bind socket.
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT_NUM);
	rc = bind(fd, (struct sockaddr *) &server, sizeof(server));
	if (rc < 0)
		err(EXIT_FAILURE, "bind");

	return (fd);
}

static void
create_thread(int fd)
{
	pthread_t thread;
	int *copy;

	// Sanity check.
	assert(fd >= 0);

	// Make a copy of the socket.
	copy = malloc(sizeof(*copy));
	if (copy == NULL)
		err(EXIT_FAILURE, "malloc");
	*copy = fd;

	// Create a new thread.
	pthread_create(&thread, NULL, client_handler, copy);
	pthread_detach(thread);
}

static void
usage(const char *name)
{
	// Sanity check.
	assert(name != NULL);

	printf("Usage: %s\n\n"
	       "Listens on TCP/UDP port %d, discarding anything it receives.\n"
	       "This program takes no arguments.\n",
	       name, PORT_NUM);

	exit(EXIT_FAILURE);
}

int
main(int argc, const char **argv)
{
	int client, fd;

	// Check command line arguments.
	if (argc != 1)
		usage(argv[0]);

	/*
	 * All UDP clients can be served by a single thread because packets are
	 * independent.
	 */
	fd = create_socket(SOCK_DGRAM);
	create_thread(fd);

	/*
	 * Each TCP connection will be served by its own thread, because packets
	 * are part of a stream.
	 */
	fd = create_socket(SOCK_STREAM);
	listen(fd, BACKLOG);
	while (true)
	{
		client = accept(fd, NULL, NULL);
		create_thread(client);
	}

	return (EXIT_SUCCESS);
}
