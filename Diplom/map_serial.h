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
#include <string>
#include <iostream>
#include <array>
#include <log_class.h>

extern Log * logfile;

struct Serial{
public:
    // Setting up the map buffer
    Serial(void){
        std::string func = "Constructor";
        logfile->write_begin(name, func);
        this->map_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "Serial_map");
        logfile->write_event(name, func, "Map file create", (int64_t)this->map_file);
        this->map_buffer = (float *)MapViewOfFile(this->map_file, FILE_MAP_ALL_ACCESS, 0, 0, 256);
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
        logfile->write_end(name, func);
    }

    // Send Com number to the slave process
    void set_com(uint8_t com, uint16_t baud){
        std::string func = "set_com";
        logfile->write_begin(name, func);
        this->map_buffer[COM_NUMBER] = com;
        logfile->write_event(name, func, "COM number", this->map_buffer[COM_NUMBER]);
        this->map_buffer[BAUD_RATE] = baud;
        logfile->write_event(name, func, "Baud Rate", this->map_buffer[BAUD_RATE]);
        logfile->write_end(name, func);
    }

    // Set connection availability
    int set_connection(void){
        std::string func = "set_connection";
        logfile->write_begin(name, func);
        uint8_t wait_time = 0;
        if (this->check_connection() != 1){
            this->map_buffer[CONN_ENABLE_SER] = 1;
            logfile->write_event(name, func, "Enable", this->map_buffer[CONN_ENABLE_SER]);
            while (this->check_connection() != 1){
                logfile->write_event(name, func, "Timeout", wait_time);
                Sleep(1000);
                if ((wait_time++) == 4){
                    this->map_buffer[CONN_ENABLE_SER] = 0;
                    logfile->write_event(name, func, "Enable", this->map_buffer[CONN_ENABLE_SER]);
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
    void disconnect(void){
        std::string func = "disconnect";
        logfile->write_begin(name, func);
        this->map_buffer[CONN_ENABLE_SER] = 0;
        logfile->write_event(name, func, "Enable", this->map_buffer[CONN_ENABLE_SER]);
        this->map_buffer[CONN_SET_SER] = 0;
        logfile->write_event(name, func, "Connection", this->map_buffer[CONN_SET_SER]);
        for (int i = 0; i < 256; i++) this->map_buffer[i] = -1;
        logfile->write_end(name, func);
    }

    // Checking for module is connected
    int check_connection(void){
        std::string func = "check_connection";
        logfile->write_begin(name, func);
        logfile->write_event(name, func, "Connection check", this->map_buffer[CONN_SET_SER]);
        logfile->write_end(name, func);
        return this->map_buffer[CONN_SET_SER];
    }

    // Gyro and Accellerometer get
    std::array<float, 6> get_gyro_accel(void){
        std::string func = "get_gyro_accel";
        logfile->write_begin(name, func);
        std::array<float, 6> output;
        if (this->check_connection() == 1){
            logfile->write_event(name, func, "Gyro Accel mode", this->map_buffer[PROCESS_TYPE]);
            // If the average computation selected
            if (this->map_buffer[PROCESS_TYPE] == MODE_AVG){
                logfile->write_event(name, func, "Gyro Accel samples", this->map_buffer[SAMPLES]);
                if (this->map_buffer[SAMPLES] != 0){
                    for (int i = 0; i < 6; i++){
                        output[i] = this->map_buffer[i + DATA_SER] / this->map_buffer[SAMPLES];
                        logfile->write_event(name, func, "Gyro Accel " + std::to_string(i), output[i]);
                        map_buffer[i + DATA_SER] = 0;
                    }
                    this->map_buffer[SAMPLES] = 0;
                    logfile->write_event(name, func, "Gyro Accel samples", this->map_buffer[SAMPLES]);
                    logfile->write_end(name, func);
                    return output;
                }
                else{
                    for (int i = 0; i < 6; i++){
                        output[i] = -1;
                        logfile->write_event(name, func, "Gyro Accel " + std::to_string(i), output[i]);
                    }
                    logfile->write_end(name, func);
                    return output;
                }
            }
            // If the true computation selected
            else{
                for (int i = 0; i < 6; i++){
                    output[i] = this->map_buffer[i + DATA_SER];
                    logfile->write_event(name, func, "Gyro Accel " + std::to_string(i), output[i]);
                }
                this->map_buffer[SAMPLES] = 0;
                logfile->write_event(name, func, "Gyro Accel samples", this->map_buffer[SAMPLES]);
                logfile->write_end(name, func);
                return output;
            }
        }
        else {
            for (auto iter = output.begin(); iter != output.end(); iter++) *iter = -1;
            for (int i = 0; i < 6; i++) logfile->write_event(name, func, "Gyro Accel " + std::to_string(i), output[i]);
            logfile->write_end(name, func);
            return output;
        }
    }

    // Set gyro and accellerometer data processing type
    void set_processing_type(uint8_t type){
        std::string func = "set_procesing_type";
        logfile->write_begin(name, func);
        this->map_buffer[PROCESS_TYPE] = type;
        logfile->write_event(name, func, "Gyro Accel mode", this->map_buffer[PROCESS_TYPE]);
        logfile->write_end(name, func);
    }

    // Destructor
    ~Serial(void){
        std::string func = "Destructor";
        logfile->write_begin(name, func);
        logfile->write_end(name, func);
        CloseHandle(this->map_file);
    }

private:
    std::string name = "SERIAL";
    float * map_buffer;
    HANDLE map_file;
};

#endif // MAP_SERIAL_H
