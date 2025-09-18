#include "UAV_Funcs.h"

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
    mavlink_status_t status;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

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
    mavlink_message_t msg;
    mavlink_status_t status;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

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

#include <fstream>

unsigned char* getImage(char *filename, int& imageSize)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return nullptr;
    }

	// Определяем размер файла
    file.seekg(0, std::ios::end);
    imageSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Выделяем память и читаем данные
    unsigned char* image = new unsigned char[imageSize];
    file.read(reinterpret_cast<char*>(image), imageSize);

	return image;
}

void sendImage(InterfaceTCPClient tmp)
{
	while (true)
	{
		int n = 0;
		unsigned char* image = getImage("drone.png", n);
		tmp.sendData(image, n);

        delete[] image;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void recvCoords(InterfaceTCPServer tmp)
{
    InterfaceUDP Autopilot(MAVLINK_IP, MAVLINK_PORT);
    waitHeartBeat(Autopilot);

    mavlink_message_t msg;

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
	InterfaceTCPServer CoordsRecv(TEST_IP, TEST_PORT + 1);
	InterfaceTCPClient ImageSend(TEST_IP, TEST_PORT);

    std::thread sendThread(sendImage, ImageSend);
    std::thread recvThread(recvCoords,  CoordsRecv);

    sendThread.join();
    recvThread.join();
}
