#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

struct BufferMMAP {
    void* start = nullptr;
    size_t length = 0;
};

class CameraV4L2 {
private:
    int fd;
    struct v4l2_format fmt{};
    std::string devPath;
    std::vector<BufferMMAP> buffers;

public:
    explicit CameraV4L2(const char* device = "/dev/video0");
    ~CameraV4L2();

    bool openDevice();
    bool initDevice(uint32_t width, uint32_t height);
    bool startCapturing();
    bool stopCapturing();
    void closeDevice();
    int frameSize() const;
    bool getFrame(uint8_t* &bufferReturn, int& n);
};


#endif