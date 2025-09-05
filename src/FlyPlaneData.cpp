#include "FlyPlaneData.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

WGS84Coord::WGS84Coord() : lat(0), lon(0), alt(0) {}
WGS84Coord::WGS84Coord(float lat, float lon, float alt) : lat(lat), lon(lon), alt(alt) {}

FlyPlaneData::FlyPlaneData()
{
    coords = nullptr;
    pointCount = 0;
}

FlyPlaneData::~FlyPlaneData() 
{
    delete[] coords;
    coords = nullptr;
}

void FlyPlaneData::setCoords(WGS84Coord* newCoords, int count) 
{
    delete[] coords;
    
    coords = new WGS84Coord[count];
    pointCount = count;
    
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

void FlyPlaneData::setImage(unsigned char *bytes)
{
    image = bytes;
}

void FlyPlaneData::xorEncryptDecrypt(unsigned char* data, size_t size) 
{
    if (!data || size == 0 || !key) return;

    size_t keyLength = strlen(key);

    for (size_t i = 0; i < size; i++)
        data[i] ^= key[i % keyLength];
}

unsigned char* FlyPlaneData::Serialization() {
    // Рассчитываем общий размер данных
    size_t totalSize = sizeof(int); // pointCount
    totalSize += sizeof(WGS84Coord) * pointCount; // coords
    totalSize += sizeof(int); // размер изображения
    if (image) {
        // Для простоты предположим, что image имеет размер pointCount * 3 (RGB)
        totalSize += pointCount * 3;
    } else {
        totalSize += 0;
    }

    // Выделяем память
    unsigned char* data = new unsigned char[totalSize];
    unsigned char* ptr = data;

    // Сериализуем pointCount
    memcpy(ptr, &pointCount, sizeof(int));
    ptr += sizeof(int);

    // Сериализуем coords
    if (coords && pointCount > 0) {
        memcpy(ptr, coords, sizeof(WGS84Coord) * pointCount);
        ptr += sizeof(WGS84Coord) * pointCount;
    }

    // Сериализуем размер изображения и само изображение
    int imageSize = image ? pointCount * 3 : 0;
    memcpy(ptr, &imageSize, sizeof(int));
    ptr += sizeof(int);

    if (image && imageSize > 0) {
        memcpy(ptr, image, imageSize);
        ptr += imageSize;
    }

    // Шифруем данные
    xorEncryptDecrypt(data, totalSize);

    return data;
}

void FlyPlaneData::DeSerialization(unsigned char* data) 
{
    if (!data) return;

    // Сначала расшифровываем данные
    // Для определения размера нам нужно знать структуру данных
    // Временная дешифровка для чтения pointCount
    int tempPointCount;
    memcpy(&tempPointCount, data, sizeof(int));
    
    // Рассчитываем ожидаемый размер для полной дешифровки
    size_t expectedSize = sizeof(int) + sizeof(WGS84Coord) * tempPointCount + sizeof(int) + tempPointCount * 3;
    xorEncryptDecrypt(data, expectedSize);

    unsigned char* ptr = data;

    // Десериализуем pointCount
    memcpy(&pointCount, ptr, sizeof(int));
    ptr += sizeof(int);

    // Освобождаем старые данные и выделяем память для новых
    delete[] coords;
    coords = new WGS84Coord[pointCount];

    // Десериализуем coords
    if (pointCount > 0) {
        memcpy(coords, ptr, sizeof(WGS84Coord) * pointCount);
        ptr += sizeof(WGS84Coord) * pointCount;
    }

    // Десериализуем размер изображения
    int imageSize;
    memcpy(&imageSize, ptr, sizeof(int));
    ptr += sizeof(int);

    // Десериализуем изображение
    delete[] image;
    if (imageSize > 0) {
        image = new unsigned char[imageSize];
        memcpy(image, ptr, imageSize);
        ptr += imageSize;
    } else {
        image = nullptr;
    }

    // Снова шифруем данные (если нужно сохранить их в зашифрованном виде)
    xorEncryptDecrypt(data, expectedSize);
}

size_t FlyPlaneData::getSerializedSize() const {
    // Базовый размер: pointCount + imageSize
    size_t totalSize = sizeof(int) * 2;
    
    // Размер для координат
    totalSize += sizeof(WGS84Coord) * pointCount;
    
    // Размер для изображения (предполагаем RGB по 1 байту на канал)
    if (image && pointCount > 0) {
        totalSize += pointCount * 3; // 3 байта на точку (R, G, B)
    }
    
    return totalSize;
}