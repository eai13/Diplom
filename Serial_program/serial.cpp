#define DISABLED 		0
#define ENABLED 		1
#define DISCONNECTED 	0
#define CONNECTED		1

#define COM_NUMBER		0
#define BAUD_RATE		1
#define CONN_ENABLE		2
#define CONN_SET		3
#define DATA			4
#define PROCESS_TYPE	10
#define SAMPLES			11

#define MODE_TRUE		0
#define MODE_AVG		1

#include "windows.h"
#include <iostream>
#include <string>

HANDLE map_file;
float * map_file_data;
HANDLE serial;
DCB params = {0};

int main(){
	// Connecting to the map file
	map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "Serial_map");
	std::cout << "Connecting to the shared map file: ";
	while(map_file == NULL) map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "Serial_map");
	std::cout << "CONNECTED" << std::endl;
	map_file_data = (float *)MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, 0, 0, 256);
	
	// Start checking the flags from the main process
	while(true){
		std::cout << "Waiting for connection command: ";
		// Waiting for the connection command from the main process
		while(map_file_data[CONN_ENABLE] != ENABLED) std::cout << "";
		std::cout << "RECEIVED" << std::endl;
		// Establishing connection
		std::string com_name = "COM" + std::to_string((int)(map_file_data[COM_NUMBER]));
		serial = ::CreateFile(com_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (serial == INVALID_HANDLE_VALUE){
			std::cout << "Connection status: FAILED" << std::endl << std::endl;
			map_file_data[CONN_SET] = DISCONNECTED;
			CloseHandle(serial);
			std::cout << "DISCONNECTED" << std::endl;
			continue;
		}
		params.DCBlength = sizeof(params);
		if (!GetCommState(serial, &params)){
			std::cout << "Connection status: FAILED" << std::endl << std::endl;
			map_file_data[CONN_SET] = DISCONNECTED;
			CloseHandle(serial);
			std::cout << "DISCONNECTED" << std::endl;
			continue;
		}
		params.BaudRate = (int)map_file_data[BAUD_RATE];
		params.ByteSize = 8;
		params.StopBits = ONESTOPBIT;
		params.Parity = NOPARITY;
		if (!SetCommState(serial, &params)){
			std::cout << "Connection status: FAILED" << std::endl << std::endl;
			map_file_data[CONN_SET] = DISCONNECTED;
			CloseHandle(serial);
			std::cout << "DISCONNECTED" << std::endl;
			continue;
		}
		else{
			std::cout << "Connection status: SUCCESSFUL" << std::endl;
			map_file_data[CONN_SET] = CONNECTED;
		}
		
		char sReceivedChar = ' ';
		uint16_t position = 0;
		char combuffer[128];
		float com_data[6];
		DWORD iSize;

		// Main data parsing cycle
		while(map_file_data[CONN_ENABLE] == ENABLED){
			// Reading from COM port
			position = 0;
			while (sReceivedChar != 'z') ReadFile(serial, &sReceivedChar, 1, &iSize, 0);
			sReceivedChar = ' ';
			while (sReceivedChar != 'z') {
				ReadFile(serial, &sReceivedChar, 1, &iSize, 0);
				combuffer[position] = sReceivedChar;
				position++;
			}
			combuffer[position] = '\0';
			// Parsing the string
			sscanf_s(combuffer, "%f\t%f\t%f\t%f\t%f\t%f\t", com_data, com_data + 1, com_data + 2, com_data + 3, com_data + 4, com_data + 5);
			// Processing, depending on the choice
			if (map_file_data[PROCESS_TYPE] == MODE_TRUE){
				for (int i = 0; i < 6; i++) map_file_data[DATA + i] = com_data[i];
				map_file_data[SAMPLES] = 1;
			}
			else if (map_file_data[PROCESS_TYPE] == MODE_AVG){
				for (int i = 0; i < 6; i++) map_file_data[DATA + i] += com_data[i];
				map_file_data[SAMPLES]++;
			}
			//std::cout << map_file_data[PROCESS_TYPE] << "   " << map_file_data[SAMPLES] << std::endl;
		}
		std::cout << "DISCONNECTED" << std::endl;
		map_file_data[CONN_SET] = DISCONNECTED;
		CloseHandle(serial);
	}
	return 0;
}