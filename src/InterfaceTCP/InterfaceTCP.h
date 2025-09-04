#ifndef INTERFACE_TCP_H
#define INTERFACE_TCP_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring> // для size_t

#include "FlyPlaneData.h"

#define BUFFER_SIZE	524288

class InterfaceTCP_Reader
{
public:
	int socket_fd;

	InterfaceTCP_Reader(const char* ip, const int port);
	~InterfaceTCP_Reader();
	
	ssize_t ReadData(uint8_t* buffer);
};

class InterfaceTCP_Sender
{
public:
	int handler;
	sockaddr_in addr;
	
	InterfaceTCP_Sender(const char* ip, const int port);
	~InterfaceTCP_Sender();
};

#endif