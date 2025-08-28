#include "InterfaceUDP.h"

InterfaceUDP::InterfaceUDP(char* ip, int port)
{
    handler = socket(AF_INET, SOCK_DGRAM, 0);
    
    sent_addr.sin_family = AF_INET;
    sent_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &sent_addr.sin_addr);

    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(port);
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    bind(handler, (sockaddr*)&recv_addr, sizeof(recv_addr));

    std::cout << "Сокет UDP создан..." << std::endl;
}

InterfaceUDP::~InterfaceUDP()
{
    if (handler != -1) close(handler);
}

int InterfaceUDP::sendTo(uint8_t buffer[], uint16_t len)
{
    sendto(handler, &buffer, len, 0, (sockaddr*)&sent_addr, sizeof(sent_addr));
}

void InterfaceUDP::sendFlyPlaneData(FlyPlaneData& data)
{
    unsigned char* serializedData = data.Serialization();
    int dataSize = data.getSerializedSize();

    sendto(handler, serializedData, dataSize, 0, (sockaddr*)&sent_addr, sizeof(sent_addr));
    
    delete[] serializedData;
}

int InterfaceUDP::readFlyPlaneData(FlyPlaneData &data, int timeoutMs)
{
    const int BUFFER_SIZE = 2048*16;
    unsigned char buffer[BUFFER_SIZE];

    socklen_t recv_len = sizeof(recv_addr);
    ssize_t bytesReceived = recvfrom(handler, buffer, BUFFER_SIZE, 0, (sockaddr*)&recv_addr, &recv_len);
    
    if (bytesReceived > 0) 
    {
        unsigned char* received_data = new unsigned char[bytesReceived];
        memcpy(received_data, buffer, bytesReceived);

        data.DeSerialization(received_data);
        
        delete[] received_data;
    }

    return bytesReceived;
}
