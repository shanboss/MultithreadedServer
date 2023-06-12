# MultithreadedServer
A simple multithreaded server in C that sends, gets, and stores data from a client. 
In order to run this code, you need to compile the dbclient.c and dbserver.c files first, using gcc dbclient.c -o dbclient and gcc dbserver.c -o dbserver.
After compiling, open one terminal window to execute the dbserver file, using ./dbserver PORT, where PORT is a port number of your choice.
In a seperate terminal, execute dbclient with ./dbclient IPAddress PORT, where IPAddress is the IP Address of the server, and PORT is the port number entered when executing the dbserver file. 
Usage: On the client side, a menu should appear where entering 1 puts name and id values to the server, entering 2 gets a name given an ID number. 0 will disconnect the client from the server. Multiple clients can be connected to server.
Because this is a very simple server program, putting and getting data from multiple clients simultaeneously will cause issues, so be sure to do those operations sequentially. All data that is entered to the server is stored in a server_data.txt file.
