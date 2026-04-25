#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <detector/client.h>
#include <detector/config.h>
#include <detector/server.h>
#include <detector/thread.h>

int main(int argc, char **argv) {
	thread_setup(THREAD_NUMBER_MAIN);

	if (argc != 2) {
		fprintf(stderr, "ERROR: Expected one argument, got %d\n", argc - 1);
		exit(EXIT_FAILURE);
	}

	const char *address = argv[1];

	int sink = open("output.hex", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (sink < 0) {
		perror("ERROR: Could not open output file");
		exit(EXIT_FAILURE);
	}

	pthread_t server;
	if (pthread_create(&server, NULL, server_main, &sink)) {
		perror("ERROR: Could not create server thread");
		exit(EXIT_FAILURE);
	}

	pthread_t client;
	if (pthread_create(&client, NULL, client_main, (void *)address)) {
		perror("ERROR: Could not create client thread");
		exit(EXIT_FAILURE);
	}

	pthread_join(server, NULL);
	pthread_join(client, NULL);

	close(sink);

	return 0;
}