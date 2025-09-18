#ifndef PC_FUNCS_H
#define PC_FUNCS_H

#include "FlyIncludes.h"
#include <fstream>

void recvImage(InterfaceTCPServer tmp);

WGS84Coord* tryReadCoords(int& count);

#endif