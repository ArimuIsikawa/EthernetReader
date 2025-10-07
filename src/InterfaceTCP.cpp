#include "InterfaceTCP.h"

InterfaceTCPServer::InterfaceTCPServer(const char *ip, const int port)
{
    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Настройка адреса
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // Привязка сокета
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Прослушивание
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", port);
}

InterfaceTCPServer::~InterfaceTCPServer()
{
    if (server_fd > 0) close(server_fd);
    if (client_fd > 0) close(client_fd);
}

int InterfaceTCPServer::recvData(uint8_t buffer[])
{
    if (ConnectToClient())
        return recv(client_fd, buffer, BUFFER_SIZE, 0);
    else
        return -1;
}

bool InterfaceTCPServer::ConnectToClient()
{
    if (client_fd <= 0) 
    {
        printf("Waiting for client connection...\n");

        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept");
            return false;
        }
        printf("Client connected\n");
    }

    return true;
}

int InterfaceTCPServer::readFlyPlaneData(FlyPlaneData &data)
{
    if (!ConnectToClient()) return -1;

    unsigned char* buffer = new unsigned char [BUFFER_SIZE];
    ssize_t bytesReceived = recv(client_fd, buffer, BUFFER_SIZE, 0);
    
    if (bytesReceived > 0) 
    {
        unsigned char* received_data = new unsigned char[bytesReceived];
        memcpy(received_data, buffer, bytesReceived);

        data.DeSerialization(received_data, bytesReceived);
        
        delete[] received_data;
        
        close(client_fd);
        client_fd = -1;
    }
    else if (bytesReceived < 0)
    {
        close(client_fd);
        client_fd = -1;
    }

    delete[] buffer;

    return bytesReceived;
}

InterfaceTCPClient::InterfaceTCPClient(const char *ip, const int port)
{
    IP = ip;
    PORT = port;
}

InterfaceTCPClient::~InterfaceTCPClient()
{
    if (sock > 0) close(sock);
}

int InterfaceTCPClient::sendData(unsigned char *data, size_t dataSize)
{
    for (int i = 0; i < 10; ++i)
    {
        if (sock > 0)
            break;

        if (sock <= 0)
            ConnectToServer();
        else
            usleep(1*100*1000);
    }
    
    if (sock > 0) 
    {
        auto result = send(sock, data, dataSize, MSG_NOSIGNAL);
        if (result == -1)
        {
            close(sock);
            sock = -1;
        }
    }
}

bool InterfaceTCPClient::ConnectToServer()
{
    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return false;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Преобразование IP адреса
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        sock = -1;
        return false;
    }
    
    // Подключение к серверу
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        //perror("Connection Failed");
        close(sock);
        sock = -1;
        return false;
    }
    
    printf("Connected to server\n");
    return true;
}

int InterfaceTCPClient::sendFlyPlaneData(FlyPlaneData& data)
{
    for (int i = 0; i < 10; ++i)
    {
        if (sock > 0)
            break;

        // Попытка подключения
        if (sock < 0)
            sock = ConnectToServer();
        else
            usleep(1*100*1000);
    }

    if (sock < 0) return -1;

    unsigned char* serializedData = data.Serialization();
    int dataSize = data.getSerializedSize();

    // Get Frame
    // Get Frame
    // Get Frame

    ssize_t bytes_sent = send(sock, serializedData, dataSize, MSG_NOSIGNAL);
    delete[] serializedData;

    if (bytes_sent <= 0)
    {
        close(sock);
        sock = -1;
    }

    return bytes_sent;
}