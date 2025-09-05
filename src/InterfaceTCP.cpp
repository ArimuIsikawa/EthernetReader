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

int InterfaceTCPServer::StartListen()
{
    // Бесконечный цикл прослушки
    while (true) 
    {
        printf("Waiting for client connection...\n");
        
        // Принятие подключения
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        printf("Client connected\n");
        
        // Получение данных
        unsigned char buffer[BUFFER_SIZE];
        ssize_t bytes_received;
        
        while (true) 
        {
            bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
            
            if (bytes_received > 0) {
                printf("Received %zd bytes:\n", bytes_received);
                for (int i = 0; i < bytes_received; i++) {
                    printf("%02X ", buffer[i]);
                }
                printf("\n");
            } 
            else if (bytes_received == 0) {
                printf("Client disconnected\n");
                break;
            }
            else {
                perror("recv error");
                break;
            }
            
            memset(buffer, 0, BUFFER_SIZE);
        }
        
        close(client_fd);
        printf("Connection closed, waiting for next client...\n\n");
    }
    
    close(server_fd);
    return 0;
}

InterfaceTCPClient::InterfaceTCPClient(const char *ip, const int port)
{
    IP = ip;
    PORT = port;

    ConnectToServer();
}

InterfaceTCPClient::~InterfaceTCPClient()
{
    if (sock > 0) close(sock);
}

int InterfaceTCPClient::ConnectToServer()
{
    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Преобразование IP адреса
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        return -1;
    }
    
    // Подключение к серверу
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        return -1;
    }
    
    printf("Connected to server\n");
    return sock;
}

int InterfaceTCPClient::SendVideoFrame()
{
    unsigned char data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x21}; // "Hello!" в hex
    size_t data_size = sizeof(data);

    for (int i = 0; i < 10; ++i)
    {
        // Попытка подключения
        if (sock < 0)
            sock = ConnectToServer();
        if (sock > 0)
            break;
        else
            usleep(1*100*1000);
    }

    if (sock < 0) return -1;

    // Get Frame
    // Get Frame
    // Get Frame
    
    // Отправка данных
    ssize_t bytes_sent = send(sock, data, data_size, MSG_NOSIGNAL);
    
    if (bytes_sent > 0) 
        printf("Sent %zd bytes\n", bytes_sent);
    else {
        printf("Send failed or connection closed. Attempting to reconnect...\n");
        close(sock);
        sock = -1;
    }

    return 0;
}
