#include "FlyPlaneData.h"
#include <cstring>
#include <stdexcept>
#include <vector>
#include <fstream>

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

void FlyPlaneData::setImage(const char* path)
{
    loadPNG(path);
}

unsigned char *FlyPlaneData::getImage()
{
    return image;
}

void FlyPlaneData::xorEncryptDecrypt(unsigned char* data, size_t size) 
{
    if (!data || size == 0 || !key) return;

    size_t keyLength = strlen(key);

    for (size_t i = 0; i < size; i++)
        data[i] ^= key[i % keyLength];
}

unsigned char* FlyPlaneData::Serialization() {
    size_t totalSize = getSerializedSize();
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

    // Сериализуем размер изображения
    memcpy(ptr, &imageSize, sizeof(size_t));
    ptr += sizeof(size_t);

    // Сериализуем данные изображения
    if (image && imageSize > 0) {
        memcpy(ptr, image, imageSize);
    }

    // Шифруем данные
    xorEncryptDecrypt(data, totalSize);

    return data;
}

bool FlyPlaneData::DeSerialization(unsigned char* ptr, size_t data_size) {
    if (!ptr || data_size < sizeof(int) + sizeof(size_t)) {
        return false;
    }

    // Создаем временную копию для дешифровки
    unsigned char* temp_data = new unsigned char[data_size];
    memcpy(temp_data, ptr, data_size);
    xorEncryptDecrypt(temp_data, data_size);

    unsigned char* temp_ptr = temp_data;
    size_t remaining_size = data_size;

    // Десериализуем pointCount
    if (remaining_size < sizeof(int)) {
        delete[] temp_data;
        return false;
    }
    memcpy(&pointCount, temp_ptr, sizeof(int));
    temp_ptr += sizeof(int);
    remaining_size -= sizeof(int);

    // Десериализуем coords
    delete[] coords;
    coords = nullptr;
    
    if (pointCount > 0) {
        size_t coords_size = sizeof(WGS84Coord) * pointCount;
        if (remaining_size < coords_size) {
            delete[] temp_data;
            return false;
        }
        coords = new WGS84Coord[pointCount];
        memcpy(coords, temp_ptr, coords_size);
        temp_ptr += coords_size;
        remaining_size -= coords_size;
    }

    // Десериализуем размер изображения
    if (remaining_size < sizeof(size_t)) {
        delete[] temp_data;
        return false;
    }
    size_t receivedImageSize;
    memcpy(&receivedImageSize, temp_ptr, sizeof(size_t));
    temp_ptr += sizeof(size_t);
    remaining_size -= sizeof(size_t);

    // Проверяем размер для данных изображения
    if (remaining_size < receivedImageSize) {
        delete[] temp_data;
        return false;
    }

    // Десериализуем данные изображения
    delete[] image;
    image = nullptr;
    imageSize = receivedImageSize;
    
    if (imageSize > 0) {
        image = new unsigned char[imageSize];
        memcpy(image, temp_ptr, imageSize);
    }

    delete[] temp_data;
    return true;
}

size_t FlyPlaneData::getSerializedSize() const {
    size_t totalSize = sizeof(int); // pointCount
    totalSize += sizeof(WGS84Coord) * pointCount;
    totalSize += sizeof(size_t); // imageSize
    totalSize += imageSize; // image data
    return totalSize;
}

bool FlyPlaneData::loadPNG(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    // Освобождаем старые данные
    delete[] image;
    image = nullptr;
    imageSize = 0;

    // Определяем размер файла
    file.seekg(0, std::ios::end);
    imageSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Выделяем память и читаем данные
    image = new unsigned char[imageSize];
    file.read(reinterpret_cast<char*>(image), imageSize);

    return file.good();
}

bool FlyPlaneData::savePNG(const char* filename)
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

size_t FlyPlaneData::getImageSize() const {
    return imageSize;
}