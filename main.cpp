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

    bool finished = false;

    while (finished != true)
    {
        ssize_t n = sitl.recvFrom(buf, sizeof(buf));
        if (n <= 0) continue;

        for (ssize_t i = 0; i < n; ++i) 
        {
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) 
            {
                switch (msg.msgid)
                {
                case MAVLINK_MSG_ID_MISSION_REQUEST:
                {
                    mavlink_mission_request_int_t req;
                    mavlink_msg_mission_request_int_decode(&msg, &req);
                    uint16_t seq = req.seq;
                    mavlink_mission_item_t wp;

                    std::cout << seq << "\n";

                    if (seq == 0)
                    {
                        missionWPTPack(wp, coords[seq], seq);
                        mavlink_msg_mission_item_encode(255, 191, &msg, &wp);
                        sendMavlinkMessage(sitl, msg);
                        break;
                    }

                    missionWPTPack(wp, coords[seq - 1], seq);
                    mavlink_msg_mission_item_encode(255, 191, &msg, &wp);
                    sendMavlinkMessage(sitl, msg);
                    break;
                }
                case MAVLINK_MSG_ID_MISSION_ACK: 
                {
                    mavlink_mission_ack_t ack;
                    mavlink_msg_mission_ack_decode(&msg, &ack);

                    if (ack.type == MAV_MISSION_ACCEPTED)
                    {
                        std::cout << "Mission uploaded successfully (ACCEPTED)." << std::endl;
                        mavlink_msg_mission_set_current_pack(255, 191, &msg, 1, 1, 0);
                        sendMavlinkMessage(sitl, msg);

                        mavlink_msg_command_long_pack(255, 191, &msg, 1, 1, 300, 0, 0.0f, 0, 0, 0, 0, 0, 0); // MAV_CMD_MISSION_START = 300
                        sendMavlinkMessage(sitl, msg);
                    }
                    else
                        std::cerr << "Mission not accepted, type=" << static_cast<int>(ack.type) << std::endl;
                    finished = true;

                    break;
                }
                default: break;
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

#include <chrono>
#include <thread>

void sendImage(InterfaceTCPClient tmp)
{
    while (true)
    {
        FlyPlaneData Data;
        Data.setImage("drone.png");
        tmp.sendFlyPlaneData(Data);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void recvImage(InterfaceTCPServer tmp)
{
    while (true)
    {
        FlyPlaneData Data;
        tmp.readFlyPlaneData(Data);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

#include <fstream>

WGS84Coord* tryReadCoords(int& count) 
{
    const char* filename = "coords.txt";
    // Открываем файл для чтения
    std::ifstream file(filename);
    
    // Проверяем, удалось ли открыть файл
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return nullptr;
    }
    
    // Перемещаем указатель в конец файла, чтобы узнать его размер
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    
    // Если файл пустой, возвращаем false
    if (fileSize == 0) {
        std::cout << "Файл пустой" << std::endl;
        file.close();
        return nullptr;
    }
    
    file.seekg(0, std::ios::beg);
    
    file >> count;
    auto coords = new WGS84Coord[count];

    for (int i = 0; i < count; ++i)
    {
        file >> coords[i].lat;
        file >> coords[i].lon;
        file >> coords[i].alt;
    }

    file.close();
    //remove(filename);

    return coords;
}


void sendCoords(InterfaceTCPClient tmp)
{
    FlyPlaneData Data;
    WGS84Coord* coords;
    int count;
    
    while (true)
    {
        coords = tryReadCoords(count);
        if (coords != nullptr)
        {
            Data.setCoords(coords, count);
            tmp.sendFlyPlaneData(Data);
        }
        count = 0;

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    delete coords;
    coords = nullptr;
}

#include "ardupilotmega.h"

void recvCoords(InterfaceTCPServer tmp)
{
    InterfaceUDP Autopilot(MAVLINK_IP, MAVLINK_PORT);
    waitHeartBeat(Autopilot);

    #ifdef __linux__
        mavlink_msg_command_long_pack(255, 191, &msg, 1, 1, MAV_CMD_COMPONENT_ARM_DISARM, 0, 1, 0, 0, 0, 0, 0, 0);
        sendMavlinkMessage(Autopilot, msg);
        usleep(1000*1000);

        mavlink_msg_command_long_pack(255, 191, &msg, 1, 1, MAV_CMD_DO_SET_MODE, 0, 209, 4, 0, 0, 0, 0, 0);
        sendMavlinkMessage(Autopilot, msg);
        usleep(1000*1000);

        mavlink_msg_command_long_pack(255, 191, &msg, 1, 1, MAV_CMD_NAV_TAKEOFF, 0, 0, 0, 0, 0, 0, 0, 5);
        sendMavlinkMessage(Autopilot, msg);
        usleep(1000*1000);
    #endif

    while (true)
    {
        FlyPlaneData Data;
        int length = tmp.readFlyPlaneData(Data);

        if (length > 0)
        {
            Do_SetWayPoints(Autopilot, Data.getCoords(), Data.getPointCount());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}


int UAV_func()
{
    InterfaceTCPServer CoordRecv(TEST_IP, TEST_PORT + 1);
    recvCoords(CoordRecv);
}

int PC_func()
{
    InterfaceTCPServer ImageRecv(TEST_IP, TEST_PORT);
    InterfaceTCPClient CoordsSend(TEST_IP, TEST_PORT + 1);

    std::thread sendThread(sendCoords, CoordsSend);
    std::thread recvThread(recvImage,  ImageRecv);

    sendThread.join();
    recvThread.join();
}

int main() 
{
    UAV_func();

    return 0;
}