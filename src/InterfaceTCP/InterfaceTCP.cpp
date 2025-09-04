#include "InterfaceTCP.h"

InterfaceTCP_Reader::InterfaceTCP_Reader(const char* ip, const int port) {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        close(socket_fd);
        throw std::runtime_error("Invalid IP address");
    }

    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(socket_fd);
        throw std::runtime_error("Failed to connect to server");
    }
}

InterfaceTCP_Reader::~InterfaceTCP_Reader()
{
    if (socket_fd > 0) close(socket_fd);
}

ssize_t InterfaceTCP_Reader::ReadData(uint8_t* buffer) {
    if (buffer == nullptr) return -1;
    
    return recv(socket_fd, buffer, sizeof(buffer), 0);
}

InterfaceTCP_Sender::InterfaceTCP_Sender(const char *ip, const int port)
{
    handler = socket(AF_INET, SOCK_STREAM, 0);
    if (handler < 0) throw std::runtime_error("Socket error\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(handler, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(handler);
        throw std::runtime_error("Connection fail\n");
    }

    std::cout << "Connection success" << std::endl;
}

InterfaceTCP_Sender::~InterfaceTCP_Sender()
{
    if (handler > 0) close(handler);
}
