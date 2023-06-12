/*
dbserver.c Written by Manu Shanbhog
Project 3 CS3377
This is a C program that listens on a specified port and accepts incoming TCP connections.
It creates a new thread for each incoming connection and passes the client socket file descriptor, client address, socket address length,
and socket family to the thread. 
The thread then calls the HandleClient function to process the incoming data from the client.

The main function first checks the command-line arguments to ensure that the correct number of arguments is given. 
It then calls the Listen function to bind to the specified port and listen for incoming connections. 
If Listen returns a value of zero or less, indicating an error, the program exits with a failure code.

The program then enters an infinite loop to accept incoming connections. 
For each incoming connection, the program allocates memory for an argument buffer to pass to the new thread. 
It then copies the client socket file descriptor, client address, socket address length, and socket family into the argument
buffer and creates a new thread to handle the incoming connection. 
After the thread is created, the main program loops back to wait for the next incoming connection.

The program is capable of recieving messages in the form of the 'record' struct, defined in msg.h. These records are written to the
server_data.txt file. Conversely, The server can retrieve the values of names given the corresponding ID number. 
*/


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "msg.h"

void Usage(char *progname);
void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);
void PrintReverseDNS(struct sockaddr *addr, size_t addrlen);
void PrintServerSide(int client_fd, int sock_family);

int  Listen(char *portnum, int *sock_family);
void HandleClient(int c_fd, struct sockaddr *addr, size_t addrlen,
                  int sock_family);


void* thread_func(void* arg) {
    int client_fd = *(int*) arg;
    struct sockaddr* caddr = (struct sockaddr*) arg + sizeof(int);
    socklen_t caddr_len = *(socklen_t*) (arg + sizeof(int) + sizeof(struct sockaddr));
    int sock_family = *(int*) (arg + sizeof(int) + sizeof(struct sockaddr) + sizeof(socklen_t));

    // call the HandleClient function with the given arguments
    HandleClient(client_fd, caddr, caddr_len, sock_family);

    // free the argument buffer
    free(arg);

    // exit the thread
    pthread_exit(NULL);
}

int 
main(int argc, char **argv) {
  // Expect the port number as a command line argument.
  if (argc != 2) {
    Usage(argv[0]);
  }

  int sock_family;
  int listen_fd = Listen(argv[1], &sock_family);
  if (listen_fd <= 0) {
    // We failed to bind/listen to a socket.  Quit with failure.
    printf("Couldn't bind to any addresses.\n");
    return EXIT_FAILURE;
  }

  // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(listen_fd,(struct sockaddr *)(&caddr),&caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      printf("Failure on accept:%s \n ", strerror(errno));
      break;
    }
   void* arg = malloc(sizeof(int) + sizeof(struct sockaddr) + sizeof(socklen_t) + sizeof(int));
    memcpy(arg, &client_fd, sizeof(int));
    memcpy(arg + sizeof(int), &caddr, sizeof(struct sockaddr));
    memcpy(arg + sizeof(int) + sizeof(struct sockaddr), &caddr_len, sizeof(socklen_t));
    memcpy(arg + sizeof(int) + sizeof(struct sockaddr) + sizeof(socklen_t), &sock_family, sizeof(int));
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, arg);
}

  // Close socket
  close(listen_fd);
  return EXIT_SUCCESS;
}

void Usage(char *progname) {
  printf("usage: %s port \n", progname);
  exit(EXIT_FAILURE);
}

void 
PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  printf("Socket [%d] is bound to: \n", fd);
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    printf(" IPv4 address %s", astring);
    printf(" and port %d\n", ntohs(in4->sin_port));

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    printf("IPv6 address %s", astring);
    printf(" and port %d\n", ntohs(in6->sin6_port));

  } else {
    printf(" ???? address and port ???? \n");
  }
}

void 
PrintReverseDNS(struct sockaddr *addr, size_t addrlen) {
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, addrlen, hostname, 1024, NULL, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
  }
  printf("DNS name: %s \n", hostname);
}

void 
PrintServerSide(int client_fd, int sock_family) {
  char hname[1024];
  hname[0] = '\0';

  printf("Server side interface is ");
  if (sock_family == AF_INET) {
    // The server is using an IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  }
}

int 
Listen(char *portnum, int *sock_family) {

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Use argv[1] as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  struct addrinfo *result;
  int res = getaddrinfo(NULL, portnum, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
	printf( "getaddrinfo failed: %s", gai_strerror(res));
    return -1;
  }

  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int listen_fd = -1;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      printf("socket() failed:%s \n ", strerror(errno));
      listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      PrintOut(listen_fd, rp->ai_addr, rp->ai_addrlen);

      // Return to the caller the address family.
      *sock_family = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_fd);
    listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (listen_fd == -1)
    return listen_fd;

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_fd, SOMAXCONN) != 0) {
    printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    close(listen_fd);
    return -1;
  }

  // Return to the client the listening file descriptor.
  return listen_fd;
}


void HandleClient(int c_fd, struct sockaddr *addr, size_t addrlen, int sock_family) {
  // Print out information about the client.
  printf("\nNew client connection \n" );
  // Loop, reading data and echo'ing it back, until the client
  // closes the connection.
  while (1) {
  char clientbuf[1024];
  ssize_t res = recv(c_fd, clientbuf, 1023, 0);
  if (res == 0) {
    printf("[The client disconnected.] \n");
    break;
  }

  if (res == -1) {
    if ((errno == EAGAIN) || (errno == EINTR))
      continue;

    printf(" Error on client socket:%s \n ", strerror(errno));
    break;
  }

  clientbuf[res] = '\0';

  int value;
  if (sscanf(clientbuf, "%d", &value) == 1) {
    if (value == 1) {
      printf("Putting Data...\n");
      
      //recieve the name and id seperately here
      struct record my_record;
      ssize_t numbytes = recv(c_fd, &my_record, sizeof(my_record), 0);
      if (numbytes == -1) {
          perror("recv");
          exit(1);
      }
      printf("Name: %s\n", my_record.name);
      printf("ID: %d\n", my_record.id);

      //Now to write to a file
      // Write the data to the file
      FILE *fp;
      fp = fopen("server_data.txt", "a");
      if (fp == NULL) {
        perror("Error opening file");
        exit(1);
      }

      fprintf(fp, "Name: %s, ID: %d\n", my_record.name, my_record.id);

      fclose(fp);
      printf("Put success.\n");


    } 
    //Get Data Here
    else if (value == 2) {
      printf("Getting Data...\n");

      //To retrieve the name given the ID
      int givenId;
      ssize_t recv_res = recv(c_fd, &givenId, sizeof(int), 0);
      if (recv_res == -1) {
        perror("recv");
        exit(1);
      } else if (recv_res == 0) {
        printf("[The client disconnected]\n");
        exit(1);
      }

      //Print the id
      printf("The value of id is %d\n", givenId);

      //Find the value of the name with the given ID:
      char name[256];
      FILE *fp = fopen("server_data.txt", "r");
      if (fp == NULL) {
        perror("Error opening file");
        exit(1);
      }
      char line[256];
      while (fgets(line, sizeof(line), fp)) {
        int id;
        char tempName[256];
        if (sscanf(line, "Name: %[^,], ID: %d", tempName, &id) == 2) {
          if (id == givenId) {
            strcpy(name, tempName);
            break;
          }
        }
        else{
          printf("No value found for the ID");
          strcpy(name, "error");
          break;
        }
      }
      fclose(fp);
      printf("The name associated with ID %d is %s\n", givenId, name);
      // Send the name value to the client
      ssize_t send_res = send(c_fd, name, strlen(name), 0);
      if (send_res == -1) {
        perror("send");
        exit(1);
      }
      


}else {
      printf("Invalid input value: %d\n", value);
    }
  } else {
    printf("The client did not send an integer\n");
  }
}
}