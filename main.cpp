#include <iostream>
#include <vector>

#include "FlyPlaneData.h"
#include "InterfaceUDP.h"
#include "InterfaceTCP.h"

#include "Mavlink_Lib/common/mavlink.h"
//#include "Mavlink_Lib/ardupilotmega/ardupilotmega.h"

#define MAIN_IP         "196.10.10.135"
#define MAIN_PORT       14597

#define TEST_IP         "127.0.0.1"
#define TEST_PORT       14519

#define MAVLINK_IP      "0.0.0.0"
#define MAVLINK_PORT    14557

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

void missionWPTPack(mavlink_mission_item_t &wp, WGS84Coord coord, int seq)
{
    wp.target_system = 1,
    wp.target_component = 1,
    wp.seq = seq;
    wp.frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
    wp.command = 16;  //MAV_CMD_NAV_WAYPOINT
    wp.current = 0;
    wp.autocontinue = 1;
    wp.x = coord.lat;  // latitude
    wp.y = coord.lon;  // longitude
    wp.z = coord.alt;  // altitude
    wp.mission_type = MAV_MISSION_TYPE_MISSION;
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

    count += 1;

    mavlink_msg_mission_count_pack(255, MAV_COMP_ID_ONBOARD_COMPUTER, &msg, 1, 1, count, MAV_MISSION_TYPE_MISSION);
    sendMavlinkMessage(sitl, msg);

    int pointI = 0;
    bool finished = false;

    while ((!(pointI < count - 1) && (finished == true)) == false)
    {
        ssize_t n = sitl.recvFrom(buf, sizeof(buf));
        if (n <= 0)
            continue;

        for (ssize_t i = 0; i < n; ++i) 
        {
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) 
            {
                switch (msg.msgid)
                {
                case MAVLINK_MSG_ID_MISSION_REQUEST: {
                    mavlink_mission_request_int_t req;
                    mavlink_msg_mission_request_int_decode(&msg, &req);
                    uint16_t seq = req.seq;

                    std::cout << seq << std::endl;

                    mavlink_mission_item_t wp;

                    if (seq == 0)
                    {
                        missionWPTPack(wp, coords[pointI], pointI);
                        mavlink_msg_mission_item_encode(255, 191, &msg, &wp);
                        sendMavlinkMessage(sitl, msg);
                        std::cout << "Sent MISSION_ITEM_INT seq=" << pointI << std::endl;
                        break;
                    }

                    missionWPTPack(wp, coords[pointI], pointI + 1);
                    mavlink_msg_mission_item_encode(255, 191, &msg, &wp);
                    sendMavlinkMessage(sitl, msg);
                    std::cout << "Sent MISSION_ITEM_INT seq=" << pointI + 1 << std::endl;
                    pointI += 1;
                    break;
                }
                case MAVLINK_MSG_ID_MISSION_ACK: {
                    mavlink_mission_ack_t ack;
                    mavlink_msg_mission_ack_decode(&msg, &ack);
                    std::cout << "Received MISSION_ACK: type=" << static_cast<int>(ack.type) << std::endl;

                    if (ack.type == MAV_MISSION_ACCEPTED)
                        std::cout << "Mission uploaded successfully (ACCEPTED)." << std::endl;
                    else
                        std::cerr << "Mission not accepted, type=" << static_cast<int>(ack.type) << std::endl;
                    finished = true;

                    break;
                }
                default:
                    break;
                }
            }
        }
    }
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

int sendData()
{
    InterfaceTCPClient InetData(TEST_IP, TEST_PORT);
    FlyPlaneData Data;
    auto tmp = new WGS84Coord[3]{
        WGS84Coord(1.0f, 2.0f, 3.0f),
        WGS84Coord(4.0f, 5.0f, 6.0f),
        WGS84Coord(7.0f, 8.0f, 9.0f)
    };

    Data.setCoords(tmp, 3);
    Data.setImage("drone.png");

    while (true)
    {
        InetData.sendFlyPlaneData(Data);

        usleep(3*1000*1000);
    }    
}

int recvData()
{
    InterfaceTCPServer InetData(TEST_IP, TEST_PORT);
    FlyPlaneData Data;

    while (true)
    {
        int length = InetData.readFlyPlaneData(Data);

        if (length > 0)
        {
            std::cout << Data.getPointCount() << std::endl;
        }
    }
}

int main() 
{
    recvData();

    return 0;

    InterfaceUDP InetData(MAIN_IP, MAIN_PORT);
    InterfaceUDP UDPSitl(MAVLINK_IP, MAVLINK_PORT);

    FlyPlaneData lastReceivedData;

    waitHeartBeat(UDPSitl);

    while (true)
    {
        int length = InetData.readFlyPlaneData(lastReceivedData);

        if (length > 0)
            Do_SetWayPoints(UDPSitl, lastReceivedData.getCoords(), lastReceivedData.getPointCount());

        usleep(1*100*1000);
    }

    return 0;
}
