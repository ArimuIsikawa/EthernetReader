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

mavlink_message_t msg;
mavlink_status_t status;
uint8_t buf[MAVLINK_MAX_PACKET_LEN];

void missionCountPack(mavlink_message_t &msg, int count)
{
    mavlink_mission_count_t m_count;
    m_count.target_system = 1;
    m_count.target_component = 1;
    m_count.count = count;
    m_count.mission_type = MAV_MISSION_TYPE_MISSION;

    mavlink_msg_mission_count_encode(255, MAV_COMP_ID_ONBOARD_COMPUTER, &msg, &m_count);
}

void missionWPTPack(mavlink_mission_item_int_t &wp, WGS84Coord coord, int seq)
{
    memset(&wp, 0, sizeof(wp));
    wp.current = 0;
    wp.target_system = 1,
    wp.target_component = 1,
    wp.seq = seq;
    wp.frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
    wp.command = MAV_CMD_NAV_WAYPOINT;
    wp.x = coord.lat;  // latitude
    wp.y = coord.lon;  // longitude
    wp.z = coord.alt;  // altitude
    wp.mission_type = MAV_MISSION_TYPE_MISSION;
    wp.autocontinue = 1;
}

void sendMavlinkMessage(InterfaceUDP &sitl, const mavlink_message_t& msg)
{
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
    
    sitl.sendTo(buffer, len);
}

void Do_SetWayPoints(InterfaceUDP &sitl, WGS84Coord* coords, int count) 
{
    mavlink_message_t msg;

    //missionCountPack(msg, count);
    //sendMavlinkMessage(sitl, msg);

    mavlink_msg_mission_count_pack(255, MAV_COMP_ID_ONBOARD_COMPUTER, &msg,
                                  1, 1,
                                  1, 0);
    sendMavlinkMessage(sitl, msg);
    usleep(1000*100);

    for (int i = 0; i < count; ++i)
    {
        mavlink_mission_item_int_t wp;
        missionWPTPack(wp, coords[i], i);

        mavlink_msg_mission_item_int_encode(255, MAV_COMP_ID_ONBOARD_COMPUTER, &msg, &wp);
        sendMavlinkMessage(sitl, msg);
        usleep(1000*100);
    }


    mavlink_msg_mission_set_current_pack(255, MAV_COMP_ID_ONBOARD_COMPUTER, &msg, 1, 1, 0);
    sendMavlinkMessage(sitl, msg);
    usleep(1000*100);
    
    mavlink_msg_mission_request_list_pack(255, MAV_COMP_ID_ONBOARD_COMPUTER, &msg, 1, 1, 0);
    sendMavlinkMessage(sitl, msg);
    usleep(1000*100);
} 

void waitHeartBeat(InterfaceUDP &sitl)
{
    while (true) 
    {
        ssize_t n = sitl.recvFrom(buf, sizeof(buf));
        if (n <= 0)
            continue;

        for (ssize_t i = 0; i < n; ++i) 
        {
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) 
            {
                if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT)
                {
                    std::cout << "Heartbeat getted\n";
                    return;
                }
            }
        }
    }
}

int main() 
{

    FlyPlaneData data;
    data.setCoords(new WGS84Coord(56.092952, 35.871884, 159.21), 1);

    InterfaceUDP UDPData(MAIN_IP, MAIN_PORT);
    InterfaceUDP UDPSitl(SITL_IP, SITL_PORT);

    FlyPlaneData lastReceivedData;
    FlyPlaneData lastSendedData;

    waitHeartBeat(UDPSitl);

    while (true)
    {
        UDPData.sendFlyPlaneData(data);

        int length = UDPData.readFlyPlaneData(lastReceivedData);

        ssize_t n = UDPSitl.recvFrom(buf, sizeof(buf));

        if (length > 0)
        {
            Do_SetWayPoints(UDPSitl, lastReceivedData.getCoords(), lastReceivedData.getPointCount());

            std::cout << lastReceivedData.getCoords()[0].lat << std::endl;
        }

        //return 0;
        usleep(1*1000*1000);
    }

    return 0;
}
