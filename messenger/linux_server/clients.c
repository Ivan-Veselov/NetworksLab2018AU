#include <stdio.h>
#include <string.h>

#include "clients.h"
#include "../common/communication.h"

void* client_interaction_routine(void* arg) {
	struct client_data *client_data = (struct client_data*)arg;

	char *buffer = malloc(sizeof(char) * (MAX_CHUNK_LEN + 1));
	bzero(buffer, sizeof(char) * (MAX_CHUNK_LEN + 1));

	if (receive_cstring(client_data->sock, buffer) > 0) {
		client_data->nickname = malloc(strlen(buffer) + 1);
		strcpy(client_data->nickname, buffer);

		while (receive_cstring(client_data->sock, buffer) > 0) {
			struct message msg = {client_data->nickname, buffer};
			broadcast_message(msg, client_data->broadcast_mutex);
		}
	}

	free(buffer);
	free(client_data->nickname);
	close(client_data->sock);
	client_data->state = REQUIRE_DELETION;

	return NULL;
}

struct client_data* find_empty_client_cell() {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i].state != INITIALIZED) {
			if (clients[i].state == REQUIRE_DELETION) {
				pthread_join(clients[i].thread, NULL);
				close(clients[i].sock);
				clients[i].state = UNITIALIZED;
			}

			return clients + i;
		}
	}

	return NULL;
}

void broadcast_message(struct message msg, pthread_mutex_t *broadcast_mutex) {
	pthread_mutex_lock(broadcast_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i].state != INITIALIZED) {
			continue;
		}

		if (send_cstring(clients[i].sock, msg.nickname) < 0 ||
			send_cstring(clients[i].sock, msg.text) < 0) {
			fprintf(stderr, "Failed to send data to socket #%d\n", clients[i].sock);
		}
	}

	pthread_mutex_unlock(broadcast_mutex);
}
