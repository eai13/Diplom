#ifndef MAP_CAMERA_H
#define MAP_CAMERA_H

// Map file:
// _________________________________________________________________________________
// |     0    |        1        |       2      |  3  |  4  |  5  |  6  |  7  |  8  |
// -----------+-----------------+--------------+------------------------------------
// |Cam number|Connection enable|Connection set|Position (X,Y,Fi)| Delta position  |
// ---------------------------------------------------------------------------------

// Data in the map file
#define POS_X               3
#define POS_Y               4
#define ANGLE               5
#define DX                  6
#define DY                  7
#define DANGLE              8

// Camera number
#define CAMERA              0
// Connection enable may be set only by the master
#define CONN_ENABLE_CAM     1
// Check if connection was set
#define CONN_SET_CAM        2
// Data starting position
#define DATA_CAM            3

#include <Windows.h>
#include <stdint.h>
#include <iostream>
#include <array>
#include <string>
#include <log_class.h>

struct Camera{
public:
    // Constructor
    Camera(void){
        // Creating mapping file
        this->map_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "Camera_map");

        // Binding it to the float array
        this->map_buffer = (float *)MapViewOfFile(this->map_file, FILE_MAP_ALL_ACCESS, 0, 0, 256);
        // Setting the map file as some random number
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
    }

    // Sets the camera number
    void set_camera_number(uint8_t number){
        this->map_buffer[CAMERA] = number;
    }

    // Sets the connection with the camera process
    uint8_t set_connection(void){
        std::cout << "Connecting camera" << std::endl;
        // Time since connection attempt started
        uint8_t wait_time = 0;
        // If no connected then send connection request and check for connection set for 30 secs
        if (this->check_connection() != 1){
            this->map_buffer[CONN_ENABLE_CAM] = 1;
            while (this->check_connection() != 1){
                Sleep(1000);
                if ((wait_time++) == 59){
                    this->map_buffer[CONN_ENABLE_CAM] = 0;
                    return 0;
                }
            }
            return 1;
        }
        else return 1;
    }

    // Disconnection request sending
    void disconnect(void){
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
        this->map_buffer[CONN_SET_CAM] = -1;
        this->map_buffer[CONN_ENABLE_CAM] = 0;
    }

    // Checking for module is connected
    uint8_t check_connection(void){
        return this->map_buffer[CONN_SET_CAM];
    }

    // Current position getter
    std::array<float, 6> get_position(void){
        if (this->check_connection()){
            std::array<float, 6> output;
            for (int i = 0; i < 6; i++){
                output[i] = this->map_buffer[i + DATA_CAM];
                this->map_buffer[i + DATA_CAM] = 0;
            }
            return output;
        }
        else{
            std::array<float, 6> output;
            for (int i = 0; i < 6; i++) output[i] = -1;
            return output;
        }
    }

    // Destructor
    ~Camera(void){
        CloseHandle(this->map_file);
    }
private:
    std::ofstream * log;
    float * map_buffer;
    HANDLE map_file;
};

#endif // MAP_CAMERA_H
