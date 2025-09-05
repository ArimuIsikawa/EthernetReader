#include "FlyPlaneData.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

WGS84Coord::WGS84Coord() : lat(0), lon(0), alt(0) {}

WGS84Coord::WGS84Coord(float lat, float lon, float alt) 
    : lat(lat), lon(lon), alt(alt) {}

// Конструкторы и деструктор
FlyPlaneData::FlyPlaneData() 
    : coords(nullptr), pointCount(0), currentPoint(0) {}

FlyPlaneData::FlyPlaneData(int count) 
    : pointCount(count), currentPoint(0) 
{
    coords = new WGS84Coord[count];
    // Инициализируем нулями
    for (int i = 0; i < count; ++i) {
        coords[i] = WGS84Coord();
    }
}

FlyPlaneData::FlyPlaneData(const FlyPlaneData& other) 
    : pointCount(other.pointCount), currentPoint(other.currentPoint) 
{
    coords = new WGS84Coord[pointCount];
    std::memcpy(coords, other.coords, pointCount * sizeof(WGS84Coord));
}

FlyPlaneData::~FlyPlaneData() 
{
    delete[] coords;
    coords = nullptr; // хорошая практика
}

// Оператор присваивания
FlyPlaneData& FlyPlaneData::operator=(const FlyPlaneData& other) 
{
    if (this != &other) {
        // Освобождаем старую память
        delete[] coords;
        
        // Копируем данные
        pointCount = other.pointCount;
        currentPoint = other.currentPoint;
        
        // Выделяем новую память и копируем координаты
        coords = new WGS84Coord[pointCount];
        std::memcpy(coords, other.coords, pointCount * sizeof(WGS84Coord));
    }
    return *this;
}

// Методы для работы с координатами
void FlyPlaneData::setCoords(WGS84Coord* newCoords, int count) 
{
    // Освобождаем старую память
    delete[] coords;
    
    // Выделяем новую память
    coords = new WGS84Coord[count];
    pointCount = count;
    
    // Копируем данные
    for (int i = 0; i < count; ++i) {
        coords[i] = newCoords[i];
    }
}

WGS84Coord* FlyPlaneData::getCoords() const 
{ 
    return coords; 
}

int FlyPlaneData::getPointCount() const 
{ 
    return pointCount; 
}

int FlyPlaneData::getCurrentPoint() const 
{ 
    return currentPoint; 
}

void FlyPlaneData::setCurrentPoint(int point) 
{ 
    if (point >= 0 && point < pointCount) {
        currentPoint = point;
    }
}

// Шифрование/дешифрование XOR
void FlyPlaneData::xorEncryptDecrypt(unsigned char* data, size_t size) 
{
    size_t keyLength = std::strlen(key);
    for (size_t i = 0; i < size; ++i) {
        data[i] ^= key[i % keyLength];
    }
}

// Сериализация
unsigned char* FlyPlaneData::Serialization() 
{
    // Рассчитываем размер данных
    size_t headerSize = sizeof(int) * 3;
    size_t coordsSize = pointCount * sizeof(WGS84Coord);
    size_t totalSize = headerSize + coordsSize;
    
    // Выделяем память
    unsigned char* data = new unsigned char[totalSize];
    unsigned char* ptr = data;
    
    // Записываем заголовок
    std::memcpy(ptr, &pointCount, sizeof(int));
    ptr += sizeof(int);
    
    std::memcpy(ptr, &currentPoint, sizeof(int));
    ptr += sizeof(int);
    
    int coordsDataSize = static_cast<int>(coordsSize);
    std::memcpy(ptr, &coordsDataSize, sizeof(int));
    ptr += sizeof(int);
    
    // Записываем координаты
    std::memcpy(ptr, coords, coordsSize);
    
    // Шифруем данные
    xorEncryptDecrypt(data, totalSize);
    
    return data;
}

// Десериализация
void FlyPlaneData::DeSerialization(unsigned char* data) 
{
    if (data == nullptr) {
        throw std::runtime_error("Null pointer provided for deserialization");
    }
    
    // Сначала копируем данные для дешифровки
    size_t headerSize = sizeof(int) * 3;
    unsigned char* headerCopy = new unsigned char[headerSize];
    std::memcpy(headerCopy, data, headerSize);
    
    // Дешифруем заголовок
    xorEncryptDecrypt(headerCopy, headerSize);
    
    // Читаем заголовок из копии
    int receivedPointCount, receivedCurrentPoint, receivedCoordsSize;
    unsigned char* ptr = headerCopy;
    
    std::memcpy(&receivedPointCount, ptr, sizeof(int));
    ptr += sizeof(int);
    
    std::memcpy(&receivedCurrentPoint, ptr, sizeof(int));
    ptr += sizeof(int);
    
    std::memcpy(&receivedCoordsSize, ptr, sizeof(int));
    
    // Проверяем корректность данных
    if (receivedPointCount < 0 || receivedCoordsSize < 0) {
        delete[] headerCopy;
        throw std::runtime_error("Invalid data format: negative values");
    }
    
    if (receivedCoordsSize != receivedPointCount * static_cast<int>(sizeof(WGS84Coord))) {
        delete[] headerCopy;
        throw std::runtime_error("Invalid data format: size mismatch");
    }
    
    // Дешифруем оставшиеся данные
    xorEncryptDecrypt(data + headerSize, receivedCoordsSize);
    
    // Освобождаем старые координаты
    delete[] coords;
    
    // Выделяем память для новых координат
    pointCount = receivedPointCount;
    currentPoint = receivedCurrentPoint;
    coords = new WGS84Coord[pointCount];
    
    // Копируем координаты
    std::memcpy(coords, data + headerSize, receivedCoordsSize);
    
    // Очищаем временную память
    delete[] headerCopy;
}

// Получение размера сериализованных данных
size_t FlyPlaneData::getSerializedSize() const 
{
    return sizeof(int) * 3 + pointCount * sizeof(WGS84Coord);
}