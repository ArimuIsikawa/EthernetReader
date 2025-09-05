#ifndef FLYPLANEDATA_H
#define FLYPLANEDATA_H

#include <cstddef> // для size_t
#include <png.h>

class WGS84Coord
{
public:
    float lat, lon, alt;
    
    WGS84Coord();
    WGS84Coord(float lat, float lon, float alt);
};

class FlyPlaneData
{
private:
    int pointCount;
    WGS84Coord* coords;
    int imageWidth;
    int imageHeight;
    unsigned char* image;
    const char* key = "uav";

    void xorEncryptDecrypt(unsigned char* data, size_t size);

public:
    FlyPlaneData();
    ~FlyPlaneData();
    
    void setCoords(WGS84Coord* newCoords, int count);
    WGS84Coord* getCoords() const;
    int getPointCount() const;
    void setImage(const char* path);
    unsigned char* getImage();

    unsigned char* Serialization();
    bool DeSerialization(unsigned char* data, size_t data_size);
    size_t getSerializedSize() const;

    bool loadPNG(const char* filename);
    bool savePNG(const char* filename);
};

#endif // FLYPLANEDATA_H