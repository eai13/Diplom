#define DISABLED				0
#define ENABLED					1
#define DISCONNECTED			0
#define CONNECTED				1

#define SPEED_MODE_LOCAL		0
#define SPEED_MODE_CARTESIAN	1

#define MODE_TRUE				0
#define MODE_AVG				1

#define IP						0
#define CONN_ENABLE				4
#define CONN_SET				5
#define SET_SPEED				6
#define SET_SPEED_TYPE			9
#define CURRENT					10
#define CURRENT_SAMPLES			17
#define CURRENT_PROC_TYPE		18
#define SPEED					19
#define TICKS					22
#define SPD_TCK_SAMPLES			25
#define SPD_TCK_PROC_TYPE		26

#include "windows.h"
#include <iostream>
#include <string>

#include "rec/robotino/com/all.h"
#include "rec/core_lt/Timer.h"
#include "rec/core_lt/utils.h"

using namespace rec::robotino::com;

HANDLE map_file;
float* map_file_data;

class MyCom : public Com {
public:
	MyCom() {}
	void errorev(Error error, const char* errorstr) {
		printf("%s\n", &errorstr[0]);
	}
	void connectev() {
		printf("connected\n");
	}
	void closeev() {
		printf("finished\n");
	}
};

MyCom com;
Motor motor[3];
OmniDrive omniDrive;

float current_wheels[3];
float current_sum;
float current_axial[3];

float ticks[3];
float speed[3];

float sign(float x) {
	if (x == 0) return 1;
	else return abs(x) / x;
}

int main() {
	// Connecting to the map file
	map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "Robotino_map");
	std::cout << "Connecting to the shared map file: ";
	while (map_file == NULL) map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "Robotino_map");
	std::cout << "CONNECTED" << std::endl;
	map_file_data = (float*)MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, NULL, NULL, 256);

	// Start checking the flags from the main process
	while (true) {
		if (com.isConnected()) com.disconnect();
		std::cout << "Waiting for connection command: ";
		// Waiting for the connection command from the main process
		while (map_file_data[CONN_ENABLE] != ENABLED) std::cout << "";
		std::cout << "RECEIVED" << std::endl;
		// Establishing connection
		std::string ip = std::to_string((int)map_file_data[IP]) + "." +
			std::to_string((int)map_file_data[IP + 1]) + "." +
			std::to_string((int)map_file_data[IP + 2]) + "." +
			std::to_string((int)map_file_data[IP + 3]);
		// Connection establishing
		com.setAddress(ip.c_str());
		std::cout << ip << std::endl;
		com.connect();
		// If the connection is set then sending the feedback to the buffer
		// Else return to the connection start and feedback the failure signal
		if (com.isConnected()) {
			map_file_data[CONN_SET] = CONNECTED;
			std::cout << "Connection status: SUCCESSFUL" << std::endl;
		}
		else {
			map_file_data[CONN_SET] = DISCONNECTED;
			std::cout << "Connection status: FAILED" << std::endl;
			std::cout << "DISCONNECTED" << std::endl;
			continue;
		}
		for (int i = 0; i < 3; i++) {
			motor[i].setComId(com.id());
			motor[i].setMotorNumber(i);
		}
		while ((map_file_data[CONN_ENABLE] == ENABLED) && (com.isConnected())) {
			// Speed setting
			std::cout << map_file_data[SET_SPEED] << " " << map_file_data[SET_SPEED + 1] << " " << map_file_data[SET_SPEED + 2] << "    " << map_file_data[SET_SPEED_TYPE] << std::endl;
			if (map_file_data[SET_SPEED_TYPE] == SPEED_MODE_LOCAL) {
				motor[0].setSpeedSetPoint(map_file_data[SET_SPEED]);
				motor[1].setSpeedSetPoint(map_file_data[SET_SPEED + 1]);
				motor[2].setSpeedSetPoint(map_file_data[SET_SPEED + 2]);
			}
			else if (map_file_data[SET_SPEED_TYPE] == SPEED_MODE_CARTESIAN) omniDrive.setVelocity(map_file_data[SET_SPEED], map_file_data[SET_SPEED + 1], map_file_data[SET_SPEED + 2]);
			// Getting data
			// CURRENT
			if (map_file_data[CURRENT_PROC_TYPE] == MODE_AVG) {
				current_sum = 0;
				// Counting current for each wheel
				for (int i = 0; i < 3; i++) {
					current_wheels[i] = motor[i].motorCurrent();
					speed[i] = motor[i].actualVelocity();
					current_sum += current_wheels[i];
					map_file_data[CURRENT + i] += current_wheels[i];
				}
				map_file_data[CURRENT + 3] = current_sum;
				// Axial current
				map_file_data[CURRENT + 4] += -0.577 * sign(speed[0]) * current_wheels[0] + 0.577 * sign(speed[2]) * current_wheels[2];
				map_file_data[CURRENT + 5] += 0.333 * sign(speed[0]) * current_wheels[0] - 0.667 * sign(speed[1]) * current_wheels[1] + 0.333 * sign(speed[2]) * current_wheels[2];
				map_file_data[CURRENT + 6] += current_sum / 3;
				// Incrementing the samples number
				map_file_data[CURRENT_SAMPLES]++;
			}
			else if (map_file_data[CURRENT_PROC_TYPE] == MODE_TRUE) {
				current_sum = 0;
				// Counting current for each wheel
				for (int i = 0; i < 3; i++) {
					current_wheels[i] = motor[i].motorCurrent();
					speed[i] = motor[i].actualVelocity();
					current_sum += current_wheels[i];
					map_file_data[CURRENT + i] = current_wheels[i];
				}
				map_file_data[CURRENT + 3] = current_sum;
				// Axial current
				map_file_data[CURRENT + 4] = -0.577 * sign(speed[0]) * current_wheels[0] + 0.577 * sign(speed[2]) * current_wheels[2];
				map_file_data[CURRENT + 5] = 0.333 * sign(speed[0]) * current_wheels[0] - 0.667 * sign(speed[1]) * current_wheels[1] + 0.333 * sign(speed[2]) * current_wheels[2];
				map_file_data[CURRENT + 6] = current_sum / 3;
				// Setting the samples number as zero
				map_file_data[CURRENT_SAMPLES] = 0;
			}
			// SPEED AND TICKS
			if (map_file_data[SPD_TCK_PROC_TYPE] == MODE_AVG) {
				for (int i = 0; i < 3; i++) {
					map_file_data[SPEED + i] += motor[i].actualVelocity();
					map_file_data[TICKS + i] += motor[i].actualPosition();
				}
				map_file_data[SPD_TCK_SAMPLES]++;
			}
			else if (map_file_data[SPD_TCK_PROC_TYPE] == MODE_TRUE) {
				for (int i = 0; i < 3; i++) {
					map_file_data[SPEED + i] = motor[i].actualVelocity();
					map_file_data[TICKS + i] = motor[i].actualPosition();
				}
				map_file_data[SPD_TCK_SAMPLES] = 0;
			}
		}
		if (com.isConnected()) com.disconnect();
		map_file_data[CONN_SET] = DISCONNECTED;
	}
	return 0;
}