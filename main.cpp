#include <iostream>
#include <vector>

#include "FlyPlaneData.h"
#include "InterfaceUDP.h"

#include "Mavlink_Lib/common/mavlink.h"
#include "Mavlink_Lib/ardupilotmega/ardupilotmega.h"

#define MAIN_IP         "127.0.0.1"
#define MAIN_PORT       14534

#define SITL_IP         "127.0.0.1"
#define SITL_PORT       14557

void missionCountPack(mavlink_message_t &msg, int count)
{
    mavlink_msg_mission_count_pack(
        255,  // system_id (отправитель)
        0,    // component_id
        &msg,
        1,    // target_system (дрон)
        0,    // target_component
        count, // количество waypoints
        MAV_MISSION_TYPE_MISSION
    );
}

void missionWPTPack(mavlink_mission_item_t &wp, WGS84Coord coord, int seq)
{
    memset(&wp, 0, sizeof(wp));
    wp.target_system = 1,
    wp.target_component = 0,
    wp.seq = seq;
    wp.frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
    wp.command = MAV_CMD_NAV_WAYPOINT;
    wp.x = coord.lat;  // latitude
    wp.y = coord.lon;  // longitude
    wp.z = coord.alt;  // altitude
    wp.autocontinue = 1;
}

void sendMavlinkMessage(InterfaceUDP sitl, const mavlink_message_t& msg)
{
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    
    sitl.sendTo(buffer, len);
}

void Do_SetWayPoints(InterfaceUDP sitl, WGS84Coord* coords, int count) 
{
    mavlink_message_t msg;

    missionCountPack(msg, count);
    sendMavlinkMessage(sitl, msg);

    for (int i = 0; i < count; ++i)
    {
        mavlink_mission_item_t wp;

        missionWPTPack(wp, coords[i], i);
        mavlink_msg_mission_item_encode(255, 0, &msg, &wp);
        sendMavlinkMessage(sitl, msg);
    }
} 

int main() 
{
    FlyPlaneData data;
    data.setCoords(new WGS84Coord(100, 125, 150), 1);

    InterfaceUDP UDPData(MAIN_IP, MAIN_PORT);
    InterfaceUDP dataSender(SITL_IP, SITL_PORT);

    FlyPlaneData lastReceivedData;
    FlyPlaneData lastSendedData;

    while (true)
    {
        UDPData.sendFlyPlaneData(data);

        int length = UDPData.readFlyPlaneData(lastReceivedData);

        if (length > 0)
        {
            //Do_SetWayPoints(dataSender, lastReceivedData.getCoords(), lastReceivedData.getPointCount());

            std::cout << lastReceivedData.getCoords()[0].lat << std::endl;
        }
        usleep(3*1000*1000);
    }

    return 0;
}
