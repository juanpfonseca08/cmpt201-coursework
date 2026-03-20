/*
Questions to answer at top of client.c:
(You should not need to change the code in client.c)
1. What is the address of the server it is trying to connect to (IP address and
port number). ip: 127.0.0.1, port 8000
2. Is it UDP or TCP? How do you know?
sfd = socket(AF_INET, SOCK_STREAM, 0); -> TCP (SOCK_DGRAM -> UDP)
3. The client is going to send some data to the server. Where does it get this
data from? How can you tell in the code?
((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) > 1
4. How does the client program end? How can you tell that in the code?
with close and exit
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUF_SIZE 64
#define ADDR "127.0.0.1"

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int main() {
  struct sockaddr_in addr;
  int sfd;
  ssize_t num_read;
  char buf[BUF_SIZE];

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
    handle_error("inet_pton");
  }

  int res = connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (res == -1) {
    handle_error("connect");
  }

  while ((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) > 1) {
    if (write(sfd, buf, num_read) != num_read) {
      handle_error("write");
    }
    printf("Just sent %zd bytes.\n", num_read);
  }

  if (num_read == -1) {
    handle_error("read");
  }

  close(sfd);
  exit(EXIT_SUCCESS);
}

// server code
//
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 64
#define PORT 8000
#define LISTEN_BACKLOG 32

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

// Shared counters for: total # messages, and counter of clients (used for
// assigning client IDs)
int total_message_count = 0;
int client_id_counter = 1;

// Mutexs to protect above global state.
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;

struct client_info {
  int cfd;
  int client_id;
};

void *handle_client(void *arg) {
  struct client_info *client = (struct client_info *)arg;
  int cfd = client->cfd;
  int client_id = client->client_id;
  free(client);
  ssize_t num_read;
  char buf[BUF_SIZE];

  while ((num_read = read(cfd, buf, BUF_SIZE)) > 0) {
    buf[num_read] = '\0';
    printf("client id %d: %s", client_id, buf);

    pthread_mutex_lock(&count_mutex);
    total_message_count++;
    printf("total message count: %d\n", total_message_count);
    pthread_mutex_unlock(&count_mutex);
  }

  if (num_read == -1)
    handle_error("read");

  if (close(cfd) == -1)
    handle_error("close");

  return NULL;
}

int main() {
  struct sockaddr_in addr;
  int sfd;

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    handle_error("bind");
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) {
    handle_error("listen");
  }

  for (;;) {
    struct client_info *client =
        (struct client_info *)malloc(sizeof(struct client_info));
    if (client == NULL)
      handle_error("malloc");

    client->cfd = accept(sfd, NULL, NULL);
    if (client->cfd == -1) {
      free(client);
      handle_error("accept");
    }

    pthread_mutex_lock(&client_id_mutex);
    client->client_id = client_id_counter++;
    pthread_mutex_unlock(&client_id_mutex);

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, client) != 0) {
      free(client);
      handle_error("pthread_create");
    }

    pthread_detach(tid);
  }

  if (close(sfd) == -1)
    handle_error("close");

  return 0;
}
