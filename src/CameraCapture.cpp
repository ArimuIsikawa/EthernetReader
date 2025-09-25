#include "CameraCapture.h"

CameraV4L2::CameraV4L2(const char* device) : devPath(device), fd(-1)
{

}

CameraV4L2::~CameraV4L2() 
{ 
    closeDevice(); 
}

bool CameraV4L2::openDevice()
{
    fd = open(devPath.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        perror("open device");
        return false;
    }
    return true;
}

bool CameraV4L2::initDevice() 
{
    if (fd < 0) return false;

    // Получаем текущий формат
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        perror("VIDIOC_G_FMT");
        return false;
    }

    // Запрос mmap буферов
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4; // запросим 4 буфера
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS");
        return false;
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory\n");
        return false;
    }

    buffers.resize(req.count);
    for (unsigned int i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF");
            return false;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            perror("mmap");
            return false;
        }
    }

    return true;
}

bool CameraV4L2::startCapturing() 
{
    if (fd < 0) return false;

    // Помещаем все буферы в очередь
    for (unsigned int i = 0; i < buffers.size(); ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            return false;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        return false;
    }
    return true;
}

bool CameraV4L2::stopCapturing() {
    if (fd < 0) return false;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("VIDIOC_STREAMOFF");
        return false;
    }
    return true;
}

void CameraV4L2::closeDevice() {
    if (fd >= 0) {
        // unmap buffers
        for (auto &b : buffers) {
            if (b.start && b.length) {
                munmap(b.start, b.length);
                b.start = nullptr;
                b.length = 0;
            }
        }
        ::close(fd);
        fd = -1;
    }
}

int CameraV4L2::frameSize() const {
    return static_cast<int>(fmt.fmt.pix.sizeimage);
}

bool CameraV4L2::getFrame(uint8_t* &bufferReturn, int& n) 
{
    n = 0;
    if (fd < 0) return false;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv;
    tv.tv_sec = 2; // таймаут 2 секунды
    tv.tv_usec = 0;

    int r = select(fd + 1, &fds, NULL, NULL, &tv);
    if (r == -1) {
        if (EINTR == errno) return false;
        perror("select");
        return false;
    }
    if (r == 0) {
        // timeout
        fprintf(stderr, "select timeout\n");
        return false;
    }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        if (errno == EAGAIN) return false;
        perror("VIDIOC_DQBUF");
        return false;
    }

    // buf.bytesused содержит длину валидных данных
    if (buf.index < 0 || static_cast<unsigned int>(buf.index) >= buffers.size()) {
        fprintf(stderr, "invalid buffer index\n");
        // обязательно повторно поставить буфер в очередь
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) perror("VIDIOC_QBUF-after-invalid");
        return false;
    }

    size_t copied = 0;
    bufferReturn = new uint8_t[buf.bytesused];
    if (bufferReturn != nullptr) {
        // копируем в пользовательский буфер
        copied = buf.bytesused;
        memcpy(bufferReturn, buffers[buf.index].start, copied);
    }
    n = static_cast<int>(buf.bytesused);

    // возвращаем буфер в очередь
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        perror("VIDIOC_QBUF");
        return false;
    }

    return true;
}