#include "FlyPlaneData.h"
#include <cstring>
#include <stdexcept>
#include <vector>

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

    // Сериализуем размеры изображения
    memcpy(ptr, &imageWidth, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &imageHeight, sizeof(int));
    ptr += sizeof(int);

    // Сериализуем само изображение (RGBA формат)
    if (image && imageWidth > 0 && imageHeight > 0) {
        size_t imageSize = imageWidth * imageHeight * 4;
        memcpy(ptr, image, imageSize);
    }

    // Шифруем данные
    xorEncryptDecrypt(data, totalSize);

    return data;
}

bool FlyPlaneData::DeSerialization(unsigned char* ptr, size_t data_size) {
    if (!ptr || data_size < sizeof(int) * 3) {
        return false;
    }

    // Создаем временную копию для дешифровки
    unsigned char* temp_data = new unsigned char[data_size];
    memcpy(temp_data, ptr, data_size);
    xorEncryptDecrypt(temp_data, data_size);

    unsigned char* temp_ptr = temp_data;

    // Проверяем минимальный размер данных
    size_t min_size = sizeof(int) * 3;
    if (data_size < min_size) {
        delete[] temp_data;
        return false;
    }

    // Десериализуем pointCount
    memcpy(&pointCount, temp_ptr, sizeof(int));
    temp_ptr += sizeof(int);

    // Проверяем размер для координат
    size_t coords_size = sizeof(WGS84Coord) * pointCount;
    if (data_size < min_size + coords_size) {
        delete[] temp_data;
        return false;
    }

    // Десериализуем coords
    delete[] coords;
    if (pointCount > 0) {
        coords = new WGS84Coord[pointCount];
        memcpy(coords, temp_ptr, coords_size);
        temp_ptr += coords_size;
    } else {
        coords = nullptr;
    }

    // Десериализуем размеры изображения
    memcpy(&imageWidth, temp_ptr, sizeof(int));
    temp_ptr += sizeof(int);
    memcpy(&imageHeight, temp_ptr, sizeof(int));
    temp_ptr += sizeof(int);

    // Десериализуем изображение
    delete[] image;
    if (imageWidth > 0 && imageHeight > 0) {
        size_t image_size = imageWidth * imageHeight * 4;
        
        // Проверяем, что осталось достаточно данных для изображения
        size_t remaining_size = data_size - (temp_ptr - temp_data);
        if (remaining_size < image_size) {
            image = nullptr;
            imageWidth = 0;
            imageHeight = 0;
        } else {
            image = new unsigned char[image_size];
            memcpy(image, temp_ptr, image_size);
        }
    } else {
        image = nullptr;
    }

    delete[] temp_data;
    return true;
}

size_t FlyPlaneData::getSerializedSize() const {
    size_t totalSize = sizeof(int) * 3; // pointCount + imageWidth + imageHeight
    totalSize += sizeof(WGS84Coord) * pointCount;
    totalSize += (image && imageWidth > 0 && imageHeight > 0) ? imageWidth * imageHeight * 4 : 0;
    return totalSize;
}

bool FlyPlaneData::loadPNG(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return false;
    }

    // Проверяем сигнатуру PNG
    png_byte header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        return false;
    }

    // Инициализируем структуры libpng
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);

    png_read_info(png, info);

    imageWidth = png_get_image_width(png, info);
    imageHeight = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Преобразуем в 8-bit RGBA
    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    // Выделяем память для изображения (RGBA - 4 канала)
    size_t row_bytes = png_get_rowbytes(png, info);
    size_t image_size = row_bytes * imageHeight;
    
    delete[] image;
    image = new unsigned char[image_size];

    // Читаем построчно
    std::vector<png_bytep> row_pointers(imageHeight);
    for (int y = 0; y < imageHeight; y++) {
        row_pointers[y] = image + y * row_bytes;
    }

    png_read_image(png, row_pointers.data());
    png_read_end(png, nullptr);

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    return true;
}

bool FlyPlaneData::savePNG(const char* filename)
{
    if (!image || imageWidth <= 0 || imageHeight <= 0) {
        return false;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);

    // Настройка параметров PNG
    png_set_IHDR(
        png,
        info,
        imageWidth, imageHeight,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png, info);

    // Записываем построчно
    size_t row_bytes = 4 * imageWidth; // RGBA = 4 bytes per pixel
    std::vector<png_bytep> row_pointers(imageHeight);
    for (int y = 0; y < imageHeight; y++) {
        row_pointers[y] = image + y * row_bytes;
    }

    png_write_image(png, row_pointers.data());
    png_write_end(png, nullptr);

    png_destroy_write_struct(&png, &info);
    fclose(fp);

    return true;
}