#ifndef INTERFACE_TCP_H
#define INTERFACE_TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "FlyPlaneData.h"

#define BUFFER_SIZE 2097152

class InterfaceTCPServer
{
public:
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    InterfaceTCPServer(const char* ip, const int port);
    ~InterfaceTCPServer();

    int recvData(uint8_t buffer[]);
    int readFlyPlaneData(FlyPlaneData &data);
};

class InterfaceTCPClient
{
private:
    const char* IP;
    int PORT;
public:
    int sock;
    sockaddr_in serv_addr;

    InterfaceTCPClient(const char* ip, const int port);
	~InterfaceTCPClient();

    int sendData(unsigned char* data, size_t dataSize);
    int ConnectToServer();
    int sendFlyPlaneData(FlyPlaneData& data);
};

#endif