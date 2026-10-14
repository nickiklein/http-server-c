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

// int showIp(char *address)
// {

//   struct addrinfo hints, *res, *p;

//   // clear hints struct and fill some address info.
//   memset(&hints, 0, sizeof hints);
//   hints.ai_family = AF_UNSPEC;
//   hints.ai_socktype = SOCK_STREAM;

//   int status = getaddrinfo(address, NULL, &hints, &res);
//   if (status != 0)
//   {
//     fprintf(stderr, "Something went wrong trying to connect to the specified addess, the error is: %s\n", gai_strerror(status));
//     return 2;
//   }

//   for (p = res; p != NULL; p = p->ai_next)
//   {
//     char presentationFormat[INET6_ADDRSTRLEN];
//     void *address;

//     if (p->ai_family == AF_INET) // IPv4
//     {
//       struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
//       address = &ipv4->sin_addr;
//     }
//     else // IPv6
//     {
//       struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
//       address = &ipv6->sin6_addr;
//     }

//     inet_ntop(p->ai_family, &address, presentationFormat, sizeof presentationFormat);
//     printf("The address in presentation format is: %s\n", presentationFormat);
//   }

//   return 0;
// }

char *readFile(const char *file_name, size_t *out_size)
{
  FILE *fp;
  char *buffer = NULL;

  fp = fopen(file_name, "rb");
  if (fp == NULL)
  {
    fprintf(stderr, "Opening file with name '%s' failed with error: %s\n", file_name, strerror(errno));
    return NULL;
  }

  if (fseek(fp, 0, SEEK_END) == -1)
  {
    fprintf(stderr, "Seeking to end of file failed with error: %s\n", strerror(errno));
    goto on_error;
  }

  long file_size = ftell(fp);
  if (file_size == -1)
  {
    fprintf(stderr, "ftell failed with error: %s\n", strerror(errno));
    goto on_error;
  }

  buffer = malloc(file_size + 1);
  if (buffer == NULL)
  {
    fprintf(stderr, "malloc failed with error: %s\n", strerror(errno));
    goto on_error;
  }

  errno = 0;
  rewind(fp);
  if (errno != 0)
  {
    fprintf(stderr, "rewind failed!\n");
    goto on_error;
  }

  if (fread(buffer, 1, file_size, fp) != (size_t)file_size)
  {
    fprintf(stderr, "fread has read an unexpected amount of bytes!\n");
    goto on_error;
  }

  fclose(fp);

  buffer[file_size] = '\0';
  *out_size = file_size;
  return buffer;

on_error:
  fclose(fp);
  free(buffer);
  return NULL;
}

void send_all(int fd, char *message, int message_length)
{
  int total_bytes_sent = 0;
  int send_rounds = 0;
  while (total_bytes_sent != message_length)
  {
    int bytes_left_to_send = message_length - total_bytes_sent;

    int bytes_sent = send(fd, message + total_bytes_sent, bytes_left_to_send, 0);
    send_rounds += 1;
    if (bytes_sent == -1)
    {
      fprintf(stderr, "Sending of message failed with error: %s\n", strerror(errno));
      return;
    }
    total_bytes_sent += bytes_sent;
  }
  printf("Sending has completed successfully in %d rounds!\n", send_rounds);
}

int main()
{

  struct addrinfo hints, *res;
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

  // lose the "Address already in use" error message
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

  while (1)
  {

    struct sockaddr_storage incoming_addr;
    socklen_t incoming_addr_length = sizeof incoming_addr;

    int new_fd = accept(socket_fd, (struct sockaddr *)&incoming_addr, &incoming_addr_length);
    if (new_fd == -1)
    {
      fprintf(stderr, "Accept failed with error: %s\n", strerror(errno));
      return 1;
    }

    printf("-----\nA connection has been accepted. The file descriptor is: %d\n", new_fd);

    if (incoming_addr.ss_family == AF_INET) // IPv4 handling
    {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)&incoming_addr;
      char ipv4_presentation[INET_ADDRSTRLEN];
      inet_ntop(ipv4->sin_family, &ipv4->sin_addr, ipv4_presentation, sizeof ipv4_presentation);
      printf("The connecting IP address is: %s, the port is: %hu\n", ipv4_presentation, ipv4->sin_port);
    }
    else // IPv6 handling
    {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&incoming_addr;
      char ipv6_presentation[INET6_ADDRSTRLEN];
      inet_ntop(ipv6->sin6_family, &ipv6->sin6_addr, ipv6_presentation, sizeof ipv6_presentation);
      printf("The connecting IP address is: %s, the port is: %hu\n", ipv6_presentation, ipv6->sin6_port);
    }

    size_t file_size = 0;
    char *buffer = readFile("very_big.txt", &file_size);
    if (buffer == NULL)
    {
      return 1;
    }

    // send a welcome message to the connected address.
    // char *welcome_message = "Welcome to my amazing HTTP server! You have connected to it successfully!";
    // int message_length = strlen(welcome_message);
    send_all(new_fd, buffer, file_size);
    free(buffer);

    char buf[100];
    int bytes_received = recv(new_fd, buf, sizeof buf, 0);
    if (bytes_received == -1)
    {
      fprintf(stderr, "Receiving of message failed with error: %s\n", strerror(errno));
      return 1;
    }
    printf("Received a message of byte size: %d\n", bytes_received);

    // close(socket_fd);
  }

  return 0;
}
