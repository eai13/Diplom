#ifndef ROBOTINO_H
#define ROBOTINO_H

// Map file for the object:
// | 0 | 1 | 2 | 3 |        4        |       5      | 6 | 7 | 8 |       9      | 10 | 11 | 12 | 13 | 14 | 15 | 16 |       17      |     18     |
// |       IP      |Connection enable|Connection set| Set Speed |Set speed type|     Current(axial and local)     |Current samples|Current type|

// | 19 | 20 | 21 | 22 | 23 | 24 |      25     |    26    |
// |     Speed    |     Ticks    |Speed samples|Speed type|

// Data positions
#define CURRENT_0               10
#define CURRENT_1               11
#define CURRENT_2               12
#define CURRENT_SUM             13
#define CURRENT_X               14
#define CURRENT_Y               15
#define CURRENT_Z               16

#define SPEED_0                 19
#define SPEED_1                 20
#define SPEED_2                 21

#define TICKS_0                 22
#define TICKS_1                 23
#define TICKS_2                 24

// Set speed mode for each wheel separately
#define SPEED_MODE_LOCAL        0
// Set speed mode for axial speed setting
#define SPEED_MODE_CARTESIAN    1

// Computation type for data
#define MODE_TRUE               0
#define MODE_AVG                1

// IP for connection to robotino
#define IP                      0
// Enabling the connection only master can rewrite this
#define CONN_ENABLE_ROB         4
// Whether the connection is set or not
#define CONN_SET_ROB            5
// Values for speed
#define SET_SPEED               6
// Type of speed (axial or for each wheel)
#define SET_SPEED_TYPE          9
// Values of current
#define CURRENT                 10
// Number of samples of current
#define CURRENT_SAMPLES         17
// Type of current computation
#define CURRENT_PROC_TYPE       18
// Current robotino speed
#define SPEED                   19
// Robotino ticks of encoder amount
#define TICKS                   22
// Amount of samples for ticks and speed
#define SPD_TCK_SAMPLES         25
// Type of ticks and speed computation
#define SPD_TCK_PROC_TYPE       26

#include "Windows.h"
#include <string>
#include <stdint.h>
#include <iostream>
#include <array>

struct Robotino{
public:
    // Setting up the map buffer
    Robotino(void){
        this->map_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "Robotino_map");
        this->map_buffer = (float *)MapViewOfFile(this->map_file, FILE_MAP_ALL_ACCESS, 0, 0, 256);
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
    }

    // Sets robotino ip (parsing the string data)
    void set_ip(std::string ip){
        int current_position = IP;
        std::string buffer = "";
        for (auto iter = ip.begin(); iter != ip.end(); iter++){
            if (*iter != '.') buffer += *iter;
            else{
                this->map_buffer[current_position] = stoi(buffer);
                current_position++;
                buffer.clear();
            }
        }
        this->map_buffer[current_position] = stoi(buffer);
    }

    // Enable connection
    uint8_t set_connection(void){
        std::cout << "Connecting robotino" << std::endl;
        uint8_t wait_time = 0;
        if (this->check_connection() != 1){
            this->map_buffer[CONN_ENABLE_ROB] = 1;
            while (this->check_connection() != 1){
                Sleep(1000);
                if ((wait_time++) == 5){
                    this->map_buffer[CONN_ENABLE_ROB] = 0;
                    return 0;
                }
            }
            return 1;
        }
        else return 1;
    }

    // Disable connection
    void disconnect(void){
        this->map_buffer[CONN_ENABLE_ROB] = 0;
        this->map_buffer[CONN_SET_ROB] = 0;
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
    }

    // Checking of the robotino is connected
    uint8_t check_connection(void){
        return this->map_buffer[CONN_SET_ROB];
    }

    // Write speed
    void set_speed_cartesian(int16_t x, int16_t y, int16_t fi){
        if (this->check_connection()){
            this->map_buffer[SET_SPEED] = x;
            this->map_buffer[SET_SPEED + 1] = y;
            this->map_buffer[SET_SPEED + 2] = fi;
        }
        else {
            for (int i = 0; i < 3; i++) this->map_buffer[SET_SPEED + i] = 0;
        }
        this->map_buffer[SET_SPEED_TYPE] = SPEED_MODE_CARTESIAN;
    }
    void set_speed_motors(int16_t v1, int16_t v2, int16_t v3){
        if (this->check_connection()){
            this->map_buffer[SET_SPEED] = v1;
            this->map_buffer[SET_SPEED + 1] = v2;
            this->map_buffer[SET_SPEED + 2] = v3;
        }
        else {
            for (int i = 0; i < 3; i++) this->map_buffer[SET_SPEED + i] = 0;
        }
        this->map_buffer[SET_SPEED_TYPE] = SPEED_MODE_LOCAL;
    }

    // Current getter
    std::array<float, 7> get_current(void){
        std::array<float, 7> output;
        if (1/*this->check_connection()*/){
            // If the average computation selected
            if (this->map_buffer[CURRENT_PROC_TYPE] == MODE_AVG){
                if (this->map_buffer[CURRENT_SAMPLES] != 0){
                    for (int i = 0; i < 7; i++){
                        output[i] = this->map_buffer[i + CURRENT] / this->map_buffer[CURRENT_SAMPLES];
                        map_buffer[i + CURRENT] = 0;
                    }
                    this->map_buffer[CURRENT_SAMPLES] = 0;
                    return output;
                }
                else{
                    for (int i = 0; i < 7; i++) output[i] = -1;
                    return output;
                }
            }
            // If the true computation selected
            else{
                for (int i = 0; i < 7; i++) output[i] = this->map_buffer[i + CURRENT];
                this->map_buffer[CURRENT_SAMPLES] = 0;
                return output;
            }
        }
        else {
            for (int i = 0; i < 7; i++) output[i] = -1;
            return output;
        }
    }
    // Current process type
    void set_current_process_type(uint8_t type){
        this->map_buffer[CURRENT_PROC_TYPE] = type;
    }

    // Encoder position getter
    std::array<float, 6> get_position(void){
        std::array<float, 6> output;
        if (1/*this->check_connection()*/){
            // If the average computation selected
            if (this->map_buffer[SPD_TCK_PROC_TYPE] == MODE_AVG){
                if (this->map_buffer[SPD_TCK_SAMPLES] != 0){
                    for (int i = 0; i < 6; i++){
                        output[i] = this->map_buffer[i + SPEED] / this->map_buffer[SPD_TCK_SAMPLES];
                        map_buffer[i + SPEED] = 0;
                    }
                    this->map_buffer[SPD_TCK_SAMPLES] = 0;
                    return output;
                }
                else{
                    for (int i = 0; i < 6; i++) output[i] = -1;
                    return output;
                }
            }
            // If the true computation selected
            else{
                for (int i = 0; i < 6; i++) output[i] = this->map_buffer[i + SPEED];
                this->map_buffer[SPD_TCK_SAMPLES] = 0;
                return output;
            }
        }
        else {
            for (int i = 0; i < 6; i++) output[i] = -1;
            return output;
        }
    }
    // Encoder position process type
    void set_encoder_position_process_type(uint8_t type){
        this->map_buffer[SPD_TCK_PROC_TYPE] = type;
    }

    // Destructor
    ~Robotino(void){
        CloseHandle(this->map_file);
    }

private:
    float * map_buffer;
    HANDLE map_file;
};

#endif // ROBOTINO_H
