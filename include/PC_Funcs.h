#ifndef PC_FUNCS_H
#define PC_FUNCS_H

#include "FlyPlaneData.h"
#include "InterfaceTCP.h"
#include "FlyDefines.h"

#include <chrono>
#include <thread>

#include <iostream>
#include <fstream>

void recvImage(InterfaceTCPServer tmp);

WGS84Coord* tryReadCoords(int& count);

bool savePNG(unsigned char* image, int imageSize, const char* filename);

#endif