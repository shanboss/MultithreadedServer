/*
dbclient.c written by Manu Shanbhog
Project 3 CS3377
The client program takes in two command-line arguments, the hostname of the server and the port number to connect to. 
It then does a DNS lookup on the hostname to retrieve the IP address of the server. If the lookup fails, the program terminates.

After retrieving the IP address, the client connects to the server using TCP/IP. If the connection fails, the program terminates.

The program then presents the user with a menu to select one of the following options:

1: Put data on the server
2: Get data from the server
0: Quit

If the user selects 1, the client sends a message to the server indicating that a put operation is to be performed. 
The client then prompts the user for the name and ID of the record to be put on the server. 
The client then creates a struct with the user-provided name and ID and sends the struct to the server.

If the user selects 2, the client sends a message to the server indicating that a get operation is to be performed. 
The client then prompts the user for the ID of the record to be retrieved from the server. 
The client then sends the ID to the server and waits for the server to send back the corresponding name of the record.

If the user selects 0, the program terminates.

If the user enters an invalid choice, the program informs the user and continues to display the menu.
*/
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

#define BUF 256

void Usage(char *progname);

int LookupName(char *name,unsigned short port,struct sockaddr_storage *ret_addr, size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,const size_t addrlen, int *ret_fd);

int main(int argc, char **argv) {
  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }

  while (1) {
    // Display the menu
    printf("Enter your choice (1 to put, 2 to get, 0 to quit):\n");
    

    // Read the user's choice
    int choice;
    scanf("%d", &choice);
    getchar(); // consume the newline character left in the input buffer

    switch (choice) {
      case 1:
        printf("Initiating Put...\n");

        //send a 1 to the server indicating a PUT is about to commence
        char *message = "1\n";
        send(socket_fd, message, strlen(message), 0);

        //Ask for name and ID
        char name[MAX_NAME_LENGTH];
        int id;
        printf("Enter the name: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0'; // remove the newline character

        printf("Enter the id: ");
        char id_str[10];
        fgets(id_str, sizeof(id_str), stdin);
        id = atoi(id_str);
        
        // Create a new struct variable and copy the name and id into its fields
        struct record my_record;
        strcpy(my_record.name, name);
        my_record.id = id;
        printf("Name: %s\n", my_record.name);
        printf("ID: %d\n", my_record.id);


        // Send the struct variable to the server
        if (send(socket_fd, &my_record, sizeof(struct record), 0) == -1) {
          perror("send");
          exit(1);
        }
        break;
        printf("Put success");


//END OF PUT

      case 2:
        
        //Get data from the server in the format of struct
        printf("Initiating Get..\n");
        //send a 2 to the server indicating a GET is about to commence
        char *message2 = "2\n";
        send(socket_fd, message2, strlen(message2), 0);

        //Ask for the ID
        int id2;
        printf("Enter the id: ");
        char id_str2[10];
        fgets(id_str2, sizeof(id_str2), stdin);
        id2 = atoi(id_str2);

        if (send(socket_fd, &id2, sizeof(int), 0) == -1) {
          perror("send");
          exit(1);
        }

        // Receive the corresponding name data from the server
        char name2[MAX_NAME_LENGTH];
        ssize_t recv_res = recv(socket_fd, name2, MAX_NAME_LENGTH, 0);
        if (recv_res == -1) {
          perror("recv");
          exit(1);
        } else if (recv_res == 0) {
          printf("[The server closed the connection]\n");
          exit(1);
        }else{
          printf("Get successful\n");
        }
        name[recv_res] = '\0';

        printf("name: %s\n", name2);
        printf("id: %d\n", id2);

        memset(name2, 0, sizeof(name2)); // set name2 to null
        break;

//END OF GET

      //EXIT
      case 0:
        // Clean up.
        close(socket_fd);
        return EXIT_SUCCESS;

      default:
        printf("Invalid choice\n");
        break;
    }
  }
}

void 
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int 
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int 
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }

  *ret_fd = socket_fd;
  return 1;
}