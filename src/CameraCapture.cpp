#include "CameraCapture.h"

bool SimpleV4L2Capture::openCamera(const char *device)
{
    fd = open(device, O_RDWR);
    return (fd != -1);
}

bool SimpleV4L2Capture::setupCamera()
{
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    
    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        return (ioctl(fd, VIDIOC_S_FMT, &format) != -1);
    }
    return true;
}

bool SimpleV4L2Capture::captureSingleFrame(uint8_t *bufferRet, int& n)
{
    uint8_t buffer[5 * 1024 * 1024];
    int bytes_read = read(fd, buffer, sizeof(buffer));
    
    if (bytes_read > 0)
    {
        bufferRet = new uint8_t[bytes_read];
        memcpy(bufferRet, buffer, bytes_read);
        return true;
    }
    else
        return false;
}

SimpleV4L2Capture::~SimpleV4L2Capture()
{
    if (fd != -1) close(fd);
}
