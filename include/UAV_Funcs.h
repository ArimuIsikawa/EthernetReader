#ifndef UAV_FUNCS_H
#define UAV_FUNCS_H

#include "FlyDefines.h"
#include "FlyPlaneData.h"
#include "InterfaceUDP.h"
#include "InterfaceTCP.h"
#include "CameraCapture.h"

#include <chrono>
#include <thread>

#include "mavlink.h"
#include "ardupilotmega.h"

void missionCountPack(mavlink_message_t &msg, int count);

void missionWPTPack(mavlink_mission_item_t &wp, WGS84Coord coord, int seq);

void sendMavlinkMessage(InterfaceUDP &sitl, const mavlink_message_t& msg);

void Do_SetWayPoints(InterfaceUDP &sitl, WGS84Coord* coords, int count);

void waitHeartBeat(InterfaceUDP &sitl);

void sendImage(InterfaceTCPClient tmp);

void recvCoords(InterfaceTCPServer tmp);

int UAV_func();

#endif