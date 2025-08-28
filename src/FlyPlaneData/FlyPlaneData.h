#ifndef FLYPLANEDATA_H
#define FLYPLANEDATA_H

#include <cstddef> // для size_t

class WGS84Coord
{
public:
    float lat, lon, alt;
    
    // Конструкторы для удобства
    WGS84Coord();
    WGS84Coord(float lat, float lon, float alt);
};

class FlyPlaneData
{
private:
    WGS84Coord* coords;
    int pointCount;
    int currentPoint;
    const char* key = "uav";

    // Вспомогательные методы
    void xorEncryptDecrypt(unsigned char* data, size_t size);

public:
    // Конструкторы и деструктор
    FlyPlaneData();
    explicit FlyPlaneData(int count); // explicit чтобы избежать неявных преобразований
    FlyPlaneData(const FlyPlaneData& other);
    ~FlyPlaneData();
    
    // Оператор присваивания
    FlyPlaneData& operator=(const FlyPlaneData& other);
    
    // Методы для работы с координатами
    void setCoords(WGS84Coord* newCoords, int count);
    WGS84Coord* getCoords() const;
    int getPointCount() const;
    int getCurrentPoint() const;
    void setCurrentPoint(int point);
    
    // Методы сериализации
    unsigned char* Serialization();
    void DeSerialization(unsigned char* data);
    size_t getSerializedSize() const;
};

#endif // FLYPLANEDATA_H