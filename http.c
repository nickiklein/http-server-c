#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdlib.h>

#define PORT "3000"

int showIp(char *address)
{

  struct addrinfo hints, *res, *p;

  // clear hints struct and fill some address info.
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(address, NULL, &hints, &res);
  if (status != 0)
  {
    fprintf(stderr, "Something went wrong trying to connect to the specified addess, the error is: %s\n", gai_strerror(status));
    return 2;
  }

  for (p = res; p != NULL; p = p->ai_next)
  {
    char presentationFormat[INET6_ADDRSTRLEN];
    void *address;

    if (p->ai_family == AF_INET) // IPv4
    {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      address = &ipv4->sin_addr;
    }
    else // IPv6
    {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      address = &ipv6->sin6_addr;
    }

    inet_ntop(p->ai_family, &address, presentationFormat, sizeof presentationFormat);
    printf("The address in presentation format is: %s\n", presentationFormat);
  }

  return 0;
}

int main(int argc, char *argv[])
{

  struct addrinfo hints, *res, *p;
  int socket_fd;

  // clear hints struct and fill some address info.
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  int status = getaddrinfo(NULL, PORT, &hints, &res);
  if (status != 0)
  {
    fprintf(stderr, "Something went wrong trying to get the address info, the error is: %s\n", gai_strerror(status));
    return 1;
  }

  socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket_fd == -1)
  {
    fprintf(stderr, "Socket initialization failed with error: %s\n", strerror(errno));
    return 1;
  }

  // lose the pesky "Address already in use" error message
  int yes = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
  {
    perror("setsockopt");
    exit(1);
  }

  if (bind(socket_fd, res->ai_addr, res->ai_addrlen) == -1)
  {
    fprintf(stderr, "Bind failed with error: %s\n", strerror(errno));
    return 1;
  }

  if (listen(socket_fd, 5))
  {
    fprintf(stderr, "Listen failed with error: %s\n", strerror(errno));
    return 1;
  }

  struct sockaddr_storage incoming_addr;
  socklen_t incoming_addr_length = sizeof incoming_addr;

  int new_fd = accept(socket_fd, (struct sockaddr *)&incoming_addr, &incoming_addr_length);
  if (new_fd == -1)
  {
    fprintf(stderr, "Accept failed with error: %s\n", strerror(errno));
    return 1;
  }

  printf("A connection has been accepted. The file descriptor is: %d\n", new_fd);

  // send a welcome message to the connected address.
  char *welcome_message = "Welcome to my amazing HTTP server! You have connected to it successfully!";
  int message_length = strlen(welcome_message);
  int bytes_sent = send(new_fd, welcome_message, message_length, 0);
  if (bytes_sent == -1)
  {
    fprintf(stderr, "Sending of welcome message failed with error: %s\n", strerror(errno));
    return 1;
  }
  if (bytes_sent != message_length)
  {
    printf("Apparently not the full welcome message was sent!\n");
  }

  char buf[100];
  int bytes_received = recv(new_fd, buf, sizeof buf, 0);
  if (bytes_received == -1)
  {
    fprintf(stderr, "Receiving of message failed with error: %s\n", strerror(errno));
    return 1;
  }
  printf("Received a message of byte size: %d\n", bytes_received);

  close(socket_fd);
  return 0;
}
