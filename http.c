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
  // printf("Sending has completed successfully in %d rounds!\n", send_rounds);
  printf("Sending has completed successfully\n");
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
    char *output_buffer = readFile("files/very_big.txt", &file_size);
    if (output_buffer == NULL)
    {
      return 1;
    }

    // send a welcome message to the connected address.
    char *welcome_message = "HTTP/1.1 200 OK\r\nContent-Language: en-US\r\nContent-Length:49\r\n\r\n<!doctype html><html><h1>Hello World!</h1></html>";
    int message_length = strlen(welcome_message);
    send_all(new_fd, welcome_message, message_length);
    // send_all(new_fd, output_buffer, file_size);
    // free(output_buffer);

    char *input_buffer;
    ssize_t input_buffer_size = 100;
    input_buffer = malloc(input_buffer_size + 1);
    if (input_buffer == NULL)
    {
      fprintf(stderr, "malloc failed with error: %s\n", strerror(errno));
      return 1;
    }
    ssize_t total_bytes_received = 0;

    while (1)
    {
      if (total_bytes_received == input_buffer_size)
      {
        char *new_input_buffer;
        new_input_buffer = realloc(input_buffer, input_buffer_size * 2 + 1);
        if (new_input_buffer == NULL)
        {
          fprintf(stderr, "realloc failed with error: %s\n", strerror(errno));
          return 1;
        }
        input_buffer_size *= 2;
        input_buffer = new_input_buffer;
      }
      ssize_t bytes_received = recv(new_fd, input_buffer + total_bytes_received, input_buffer_size - total_bytes_received, 0);
      if (bytes_received == -1)
      {
        fprintf(stderr, "Receiving of message failed with error: %s\n", strerror(errno));
        free(input_buffer);
        close(new_fd);
        return 1;
      }
      else if (bytes_received == 0)
      {
        printf("The client has finished sending sending a message. Closing the socket with this client now.\n");
        // TODO: It might be the case here that the client has stopped sending but still wants to listen. How to handle that?
        close(new_fd);
        break;
      }
      total_bytes_received += bytes_received;
      input_buffer[total_bytes_received] = '\0';
      // printf("Received a message of byte size: %zd\n", bytes_received);
      // printf("The accumulated message reads: %s", input_buffer);
    }
    printf("Received a total amount of bytes of: %zd\n", total_bytes_received);
    printf("The fully received message reads: \n\n%s\n", input_buffer);
    free(input_buffer);

    // close(socket_fd);
  }

  return 0;
}
