#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring> // для size_t

#include "FlyPlaneData.h"

class InterfaceUDP
{
	int handler;
	sockaddr_in sent_addr, recv_addr;

public:
	InterfaceUDP(char* ip, int port); 
	~InterfaceUDP();

	int sendTo(uint8_t buffer[], uint16_t len);

	void sendFlyPlaneData(FlyPlaneData& data);
	int readFlyPlaneData(FlyPlaneData& data, int timeoutMs = 1000);
};