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

extern Log * logfile;

struct Camera{
public:
    // Constructor
    Camera(void){
        name = "CAMERA";
        std::string func = "Constructor";
        logfile->write_begin(name, func);
        // Creating mapping file
        this->map_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "Camera_map");
        logfile->write_event(name, func, "Map file create", (int64_t)this->map_file);
        // Binding it to the float array
        this->map_buffer = (float *)MapViewOfFile(this->map_file, FILE_MAP_ALL_ACCESS, 0, 0, 256);
        // Setting the map file as some random number
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
        logfile->write_end(name, func);
    }

    // Sets the camera number
    void set_camera_number(uint8_t number){
        std::string func = "set_camera_number";
        logfile->write_begin(name, func);
        logfile->write_event(name, func, "Camera number", this->map_buffer[CAMERA]);
        this->map_buffer[CAMERA] = number;
        logfile->write_end(name, func);
    }

    // Sets the connection with the camera process
    uint8_t set_connection(void){
        std::string func = "set_connection";
        logfile->write_begin(name, func);
        // Time since connection attempt started
        uint8_t wait_time = 0;
        // If no connected then send connection request and check for connection set for 30 secs
        if (this->check_connection() != 1){
            this->map_buffer[CONN_ENABLE_CAM] = 1;
            logfile->write_event(name, func, "Enable", this->map_buffer[CONN_ENABLE_CAM]);
            while (this->check_connection() != 1){
                logfile->write_event(name, func, "Timeout", wait_time);
                Sleep(1000);
                if ((wait_time++) == 59){
                    this->map_buffer[CONN_ENABLE_CAM] = 0;
                    logfile->write_event(name, func, "Enable", this->map_buffer[CONN_ENABLE_CAM]);
                    logfile->write_end(name, func);
                    return 0;
                }
            }
            logfile->write_event(name, func, "Connected", 1);
            logfile->write_end(name, func);
            return 1;
        }
        else{
            logfile->write_event(name, func, "Already connected", 1);
            logfile->write_end(name, func);
            return 1;
        }
    }

    // Disconnection request sending
    void disconnect(void){
        std::string func = "disconnect";
        logfile->write_begin(name, func);
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
        this->map_buffer[CONN_SET_CAM] = -1;
        this->map_buffer[CONN_ENABLE_CAM] = 0;
        logfile->write_event(name, func, "Cam", this->map_buffer[CONN_SET_CAM]);
        logfile->write_event(name, func, "Enable", this->map_buffer[CONN_ENABLE_CAM]);
        logfile->write_end(name, func);
    }

    // Checking for module is connected
    uint8_t check_connection(void){
        std::string func = "check_connection";
        logfile->write_begin(name,func);
        logfile->write_event(name, func, "Connection check", this->map_buffer[CONN_SET_CAM]);
        logfile->write_end(name, func);
        return this->map_buffer[CONN_SET_CAM];
    }

    // Current position getter
    std::array<float, 6> get_position(void){
        std::string func = "get_position";
        logfile->write_begin(name, func);
        if (this->check_connection()){
            std::array<float, 6> output;
            for (int i = 0; i < 6; i++){
                output[i] = this->map_buffer[i + DATA_CAM];
                logfile->write_event(name, func, "position[" + std::to_string(i) + "]", output[i]);
                this->map_buffer[i + DATA_CAM] = 0;
            }
            logfile->write_end(name, func);
            return output;
        }
        else{
            std::array<float, 6> output;
            for (int i = 0; i < 6; i++) output[i] = -1;
            logfile->write_end(name, func);
            return output;
        }
    }

    // Destructor
    ~Camera(void){
        std::string func = "Destructor";
        logfile->write_begin(name, func);
        logfile->write_end(name, func);
        CloseHandle(this->map_file);
    }

private:
    std::string name = "CAMERA";
    float * map_buffer;
    HANDLE map_file;
};

#endif // MAP_CAMERA_H
