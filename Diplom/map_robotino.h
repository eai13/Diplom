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
#include <log_class.h>

extern Log * logfile;

struct Robotino{
public:
    // Setting up the map buffer
    Robotino(void){
        name = "ROBOTINO";
        std::string func = "Constructor";
        logfile->write_begin(name, func);
        this->map_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "Robotino_map");
        logfile->write_event(name, func, "Map file create", (int64_t)this->map_file);
        this->map_buffer = (float *)MapViewOfFile(this->map_file, FILE_MAP_ALL_ACCESS, 0, 0, 256);
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
        logfile->write_end(name, func);
    }

    // Sets robotino ip (parsing the string data)
    void set_ip(std::string ip){
        std::string func = "set_ip";
        logfile->write_begin(name, func);
        int current_position = IP;
        std::string buffer = "";
        for (auto iter = ip.begin(); iter != ip.end(); iter++){
            if (*iter != '.') buffer += *iter;
            else{
                this->map_buffer[current_position] = stoi(buffer);
                logfile->write_event(name, func, "IP", this->map_buffer[current_position]);
                current_position++;
                buffer.clear();
            }
        }
        this->map_buffer[current_position] = stoi(buffer);
        logfile->write_event(name, func, "IP", this->map_buffer[current_position]);
        logfile->write_end(name, func);
    }

    // Enable connection
    uint8_t set_connection(void){
        std::string func = "set_connection";
        logfile->write_begin(name, func);
        uint8_t wait_time = 0;
        if (this->check_connection() != 1){
            this->map_buffer[CONN_ENABLE_ROB] = 1;
            logfile->write_event(name, func, "Enable", this->map_buffer[CONN_ENABLE_ROB]);
            while (this->check_connection() != 1){
                logfile->write_event(name, func, "Timeout", wait_time);
                Sleep(1000);
                if ((wait_time++) == 5){
                    this->map_buffer[CONN_ENABLE_ROB] = 0;
                    logfile->write_event(name, func, "Connected", (int)this->map_buffer[CONN_ENABLE_ROB]);
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

    // Disable connection
    void disconnect(void){
        std::string func = "disconnect";
        logfile->write_begin(name, func);
        this->map_buffer[CONN_ENABLE_ROB] = 0;
        logfile->write_event(name, func, "Enable", (int)this->map_buffer[CONN_ENABLE_ROB]);
        this->map_buffer[CONN_SET_ROB] = 0;
        logfile->write_event(name, func, "Connection", (int)this->map_buffer[CONN_SET_ROB]);
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
        logfile->write_end(name, func);
    }

    // Checking of the robotino is connected
    uint8_t check_connection(void){
        std::string func = "check_connection";
        logfile->write_begin(name, func);
        logfile->write_event(name, func, "Connection check", (int)this->map_buffer[CONN_SET_ROB]);
        logfile->write_end(name, func);
        return this->map_buffer[CONN_SET_ROB];
    }

    // Write speed
    void set_speed_cartesian(int16_t x, int16_t y, int16_t fi){
        std::string func = "set_speed_cartesian";
        logfile->write_begin(name, func);
        if (this->check_connection()){
            this->map_buffer[SET_SPEED] = x;
            logfile->write_event(name, func, "Speed 0", this->map_buffer[SET_SPEED]);
            this->map_buffer[SET_SPEED + 1] = y;
            logfile->write_event(name, func, "Speed 1", this->map_buffer[SET_SPEED + 1]);
            this->map_buffer[SET_SPEED + 2] = fi;
            logfile->write_event(name, func, "Speed 2", this->map_buffer[SET_SPEED + 2]);
        }
        else {
            for (int i = 0; i < 3; i++){
                this->map_buffer[SET_SPEED + i] = 0;
                logfile->write_event(name, func, "Speed " + std::to_string(i), this->map_buffer[SET_SPEED + i]);
            }
        }
        this->map_buffer[SET_SPEED_TYPE] = SPEED_MODE_CARTESIAN;
        logfile->write_event(name, func, "Speed mode", this->map_buffer[SET_SPEED_TYPE]);
        logfile->write_end(name, func);
    }
    void set_speed_motors(int16_t v1, int16_t v2, int16_t v3){
        std::string func = "set_speed_motors";
        logfile->write_begin(name, func);
        if (this->check_connection()){
            this->map_buffer[SET_SPEED] = v1;
            this->map_buffer[SET_SPEED + 1] = v2;
            this->map_buffer[SET_SPEED + 2] = v3;
            for (int i = 0; i < 2; i++) logfile->write_event(name, func, "Speed " + std::to_string(i), this->map_buffer[SET_SPEED + i]);
        }
        else {
            for (int i = 0; i < 3; i++){
                this->map_buffer[SET_SPEED + i] = 0;
                logfile->write_event(name, func, "Speed " + std::to_string(i), this->map_buffer[SET_SPEED + i]);
            }
        }
        this->map_buffer[SET_SPEED_TYPE] = SPEED_MODE_LOCAL;
        logfile->write_event(name, func, "Speed mode", this->map_buffer[SET_SPEED_TYPE]);
        logfile->write_end(name, func);
    }

    // Current getter
    std::array<float, 7> get_current(void){
        std::string func = "get_current";
        logfile->write_begin(name, func);
        std::array<float, 7> output;
        if (this->check_connection()){
            logfile->write_event(name, func, "Current mode", this->map_buffer[CURRENT_PROC_TYPE]);
            // If the average computation selected
            if (this->map_buffer[CURRENT_PROC_TYPE] == MODE_AVG){
                logfile->write_event(name, func, "Current samples", this->map_buffer[CURRENT_SAMPLES]);
                if (this->map_buffer[CURRENT_SAMPLES] != 0){
                    for (int i = 0; i < 7; i++){
                        output[i] = this->map_buffer[i + CURRENT] / this->map_buffer[CURRENT_SAMPLES];
                        logfile->write_event(name, func, "Current " + std::to_string(i), output[i]);
                        map_buffer[i + CURRENT] = 0;
                    }
                    this->map_buffer[CURRENT_SAMPLES] = 0;
                    logfile->write_event(name, func, "Current samples", this->map_buffer[CURRENT_SAMPLES]);
                    logfile->write_end(name, func);
                    return output;
                }
                else{
                    for (int i = 0; i < 7; i++){
                        output[i] = -1;
                        logfile->write_event(name, func, "Current " + std::to_string(i), output[i]);
                    }
                    logfile->write_end(name, func);
                    return output;
                }
            }
            // If the true computation selected
            else{
                for (int i = 0; i < 7; i++){
                    output[i] = this->map_buffer[i + CURRENT];
                    logfile->write_event(name, func, "Current " + std::to_string(i), output[i]);
                }
                this->map_buffer[CURRENT_SAMPLES] = 0;
                logfile->write_event(name, func, "Current samples", this->map_buffer[CURRENT_SAMPLES]);
                logfile->write_end(name, func);
                return output;
            }
        }
        else {
            for (int i = 0; i < 7; i++){
                output[i] = -1;
                logfile->write_event(name, func, "Current " + std::to_string(i), output[i]);
            }
            logfile->write_end(name, func);
            return output;
        }
    }
    // Current process type
    void set_current_process_type(uint8_t type){
        std::string func = "set_current_process_type";
        logfile->write_begin(name, func);
        this->map_buffer[CURRENT_PROC_TYPE] = type;
        logfile->write_event(name, func, "Current process type", this->map_buffer[CURRENT_PROC_TYPE]);
        logfile->write_end(name, func);
    }

    // Encoder position getter
    std::array<float, 6> get_position(void){
        std::string func = "get_position";
        logfile->write_begin(name, func);
        std::array<float, 6> output;
        if (this->check_connection()){
            logfile->write_event(name, func, "Speed Position mode", this->map_buffer[SPD_TCK_PROC_TYPE]);
            // If the average computation selected
            if (this->map_buffer[SPD_TCK_PROC_TYPE] == MODE_AVG){
                logfile->write_event(name, func, "Speed Position samples", this->map_buffer[SPD_TCK_SAMPLES]);
                if (this->map_buffer[SPD_TCK_SAMPLES] != 0){
                    for (int i = 0; i < 6; i++){
                        output[i] = this->map_buffer[i + SPEED] / this->map_buffer[SPD_TCK_SAMPLES];
                        logfile->write_event(name, func, "Speed  Position " + std::to_string(i), output[i]);
                        map_buffer[i + SPEED] = 0;
                    }
                    this->map_buffer[SPD_TCK_SAMPLES] = 0;
                    logfile->write_event(name, func, "Speed Position samples", this->map_buffer[SPD_TCK_SAMPLES]);
                    logfile->write_end(name, func);
                    return output;
                }
                else{
                    for (int i = 0; i < 6; i++){
                        output[i] = -1;
                        logfile->write_event(name, func, "Speed Position " + std::to_string(i), output[i]);
                    }
                    logfile->write_end(name, func);
                    return output;
                }
            }
            // If the true computation selected
            else{
                for (int i = 0; i < 6; i++){
                    output[i] = this->map_buffer[i + SPEED];
                    logfile->write_event(name, func, "Speed Position " + std::to_string(i), output[i]);
                }
                this->map_buffer[SPD_TCK_SAMPLES] = 0;
                logfile->write_event(name, func, "Speed Position samples", this->map_buffer[SPD_TCK_SAMPLES]);
                logfile->write_end(name, func);
                return output;
            }
        }
        else {
            for (int i = 0; i < 6; i++){
                output[i] = -1;
                logfile->write_event(name, func, "Speed Position " + std::to_string(i), output[i]);
            }
            logfile->write_end(name, func);
            return output;
        }
    }
    // Encoder position process type
    void set_encoder_position_process_type(uint8_t type){
        std::string func = "set_encoder_position_process_type";
        logfile->write_begin(name, func);
        this->map_buffer[SPD_TCK_PROC_TYPE] = type;
        logfile->write_event(name, func, "Speed Position mode", this->map_buffer[SPD_TCK_PROC_TYPE]);
        logfile->write_end(name, func);
    }

    // Destructor
    ~Robotino(void){
        std::string func = "Destructor";
        logfile->write_begin(name, func);
        logfile->write_end(name, func);
        CloseHandle(this->map_file);
    }

private:
    std::string name = "ROBOTINO";
    float * map_buffer;
    HANDLE map_file;
};

#endif // ROBOTINO_H
