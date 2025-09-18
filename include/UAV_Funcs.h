#ifndef UAV_FUNCS_H
#define UAV_FUNCS_H

#include "FlyIncludes.h"

mavlink_message_t msg;
mavlink_status_t status;
uint8_t buf[MAVLINK_MAX_PACKET_LEN];

void missionCountPack(mavlink_message_t &msg, int count);

void missionWPTPack(mavlink_mission_item_t &wp, WGS84Coord coord, int seq);

void sendMavlinkMessage(InterfaceUDP &sitl, const mavlink_message_t& msg);

void Do_SetWayPoints(InterfaceUDP &sitl, WGS84Coord* coords, int count);

void waitHeartBeat(InterfaceUDP &sitl);

void sendImage(InterfaceTCPClient tmp);

void recvCoords(InterfaceTCPServer tmp);

int UAV_func();

#endif