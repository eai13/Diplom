#ifndef MAP_SERIAL_H
#define MAP_SERIAL_H

// The map file for this object looks like this:
// |       0       |    1    |         2         |       3      | 4 | 5 | 6 | 7 |  8  | 9 |     10     |      11     |
// |Com port number|Baud rate|Connection enabling|Connection set|Gyro(X,Y,Z)|Accell(X,Y,Z)|Process type|Samples count|

// Positions for gyro and accel in the map file
#define GYRO_X                  4
#define GYRO_Y                  5
#define GYRO_Z                  6
#define ACCEL_X                 7
#define ACCEL_Y                 8
#define ACCEL_Z                 9

// Computation type for data
#define MODE_TRUE               0
#define MODE_AVG                1

// Com number position
#define COM_NUMBER              0
// Baud rate position
#define BAUD_RATE               1
// Enabling the connection only master can rewrite this
#define CONN_ENABLE_SER         2
// Checking if connection is set
#define CONN_SET_SER            3
// Gyro and accellerometer position
#define DATA_SER                4
// Data computation type
#define PROCESS_TYPE            10
// Samples amount for data
#define SAMPLES                 11

#include <Windows.h>
#include <stdint.h>
#include <iostream>
#include <array>

struct Serial{
public:
    // Setting up the map buffer
    Serial(void){
        this->map_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "Serial_map");
        this->map_buffer = (float *)MapViewOfFile(this->map_file, FILE_MAP_ALL_ACCESS, 0, 0, 256);
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
    }

    // Send Com number to the slave process
    void set_com(uint8_t com, uint16_t baud){
        this->map_buffer[COM_NUMBER] = com;
        this->map_buffer[BAUD_RATE] = baud;
    }

    // Set connection availability
    int set_connection(void){
        std::cout << "Connecting serial" << std::endl;
        uint8_t wait_time = 0;
        if (this->check_connection() != 1){
            this->map_buffer[CONN_ENABLE_SER] = 1;
            while (this->check_connection() != 1){
                Sleep(1000);
                if ((wait_time++) == 4){
                    this->map_buffer[CONN_ENABLE_SER] = 0;
                    return 0;
                }
            }
            return 1;
        }
        else return 1;
    }
    void disconnect(void){
        this->map_buffer[CONN_ENABLE_SER] = 0;
        this->map_buffer[CONN_SET_SER] = 0;
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
    }

    // Checking for module is connected
    int check_connection(void){
        return this->map_buffer[CONN_SET_SER];
    }

    // Gyro and Accellerometer get
    std::array<float, 6> get_gyro_accel(void){
        std::array<float, 6> output;
        if (this->check_connection() == 1){
            // If the average computation selected
            if (this->map_buffer[PROCESS_TYPE] == MODE_AVG){
                if (this->map_buffer[SAMPLES] != 0){
                    for (int i = 0; i < 6; i++){
                        output[i] = this->map_buffer[i + DATA_SER] / this->map_buffer[SAMPLES];
                        map_buffer[i + DATA_SER] = 0;
                    }
                    this->map_buffer[SAMPLES] = 0;
                    return output;
                }
                else{
                    for (int i = 0; i < 6; i++) output[i] = -1;
                    return output;
                }
            }
            // If the true computation selected
            else{
                for (int i = 0; i < 6; i++) output[i] = this->map_buffer[i + DATA_SER];
                this->map_buffer[SAMPLES] = 0;
                return output;
            }
        }
        else {
            for (auto iter = output.begin(); iter != output.end(); iter++) *iter = -1;
            return output;
        }
    }

    // Set gyro and accellerometer data processing type
    void set_processing_type(uint8_t type){
        this->map_buffer[PROCESS_TYPE] = type;
    }

    // Destructor
    ~Serial(void){
        CloseHandle(this->map_file);
    }

private:
    float * map_buffer;
    HANDLE map_file;
};

#endif // MAP_SERIAL_H
