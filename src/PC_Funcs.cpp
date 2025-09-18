#include "PC_Funcs.h"

WGS84Coord* tryReadCoords(int& count) 
{
    const char* filename = "coords.txt";
    // Открываем файл для чтения
    std::ifstream file(filename);
    
    // Проверяем, удалось ли открыть файл
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return nullptr;
    }
    
    // Перемещаем указатель в конец файла, чтобы узнать его размер
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    
    // Если файл пустой, возвращаем false
    if (fileSize == 0) {
        std::cout << "Файл пустой" << std::endl;
        file.close();
        return nullptr;
    }
    
    file.seekg(0, std::ios::beg);
    
    file >> count;
    auto coords = new WGS84Coord[count];

    for (int i = 0; i < count; ++i)
    {
        file >> coords[i].lat;
        file >> coords[i].lon;
        file >> coords[i].alt;
    }

    file.close();
    remove(filename);

    return coords;
}

bool savePNG(unsigned char *image, int imageSize, const char *filename)
{
    if (!image || imageSize == 0) {
        return false;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(image), imageSize);
    return file.good();
}

void sendCoords(InterfaceTCPClient tmp)
{
    FlyPlaneData Data;
    WGS84Coord* coords;
    int count;
    
    while (true)
    {
        coords = tryReadCoords(count);
        if (coords != nullptr)
        {
            Data.setCoords(coords, count);
            tmp.sendFlyPlaneData(Data);
        }
        count = 0;

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    delete coords;
    coords = nullptr;
}

void recvImage(InterfaceTCPServer tmp)
{
    while (true)
    {
		uint8_t* buffer = new uint8_t[BUFFER_SIZE]; 
        auto imageSize = tmp.recvData(buffer);
        std::cout << imageSize << std::endl;
        
		savePNG(buffer, imageSize, "getted.png");

		delete[] buffer;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void PC_func(void)
{
    InterfaceTCPServer ImageRecv(TEST_IP, TEST_PORT);
    InterfaceTCPClient CoordsSend(TEST_IP, TEST_PORT + 1);

    std::thread sendThread(sendCoords, CoordsSend);
    std::thread recvThread(recvImage,  ImageRecv);

    sendThread.join();
    recvThread.join();
}
