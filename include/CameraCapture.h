#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>
#include <csignal>

class SimpleV4L2Capture {
private:
    int fd;
    
public:
    bool openCamera(const char* device = "/dev/video0");
    bool setupCamera();

    bool captureSingleFrame(uint8_t *bufferRet, int& n);
    
    ~SimpleV4L2Capture();
};


#endif