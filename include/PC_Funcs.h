#ifndef PC_FUNCS_H
#define PC_FUNCS_H

#include "FlyPlaneData.h"
#include "InterfaceTCP.h"
#include "FlyDefines.h"

#include <chrono>
#include <thread>

#include <iostream>
#include <fstream>

WGS84Coord* tryReadCoords(int& count);

bool savePNG(unsigned char* image, int imageSize, const char* filename);

void sendCoords(InterfaceTCPClient tmp);

void recvImage(InterfaceTCPServer tmp);

void PC_func(void);

#endif