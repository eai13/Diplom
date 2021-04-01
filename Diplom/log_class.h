#ifndef LOG_CLASS_H
#define LOG_CLASS_H

#include <string>
#include <fstream>

struct Log{
    // Contructor
    Log(std::string filename){
        this->log_file.open(filename);
        if (this->log_file.is_open()){
            this->log_file << "[Log] {Constructor}" << is << "Initialization +" << std::endl;
        }
        else {
            this->log_file << "[Log] {Constructor}" << is << "Initialization -" << std::endl;
        }
    }

    // Write the event
    void write_event(std::string object, std::string func, std::string event, int result){
        this->log_file << sq(object) + fg(func) + is + ev(event) << result << std::endl;
    }
    // For functions start and end
    void write_begin(std::string object, std::string func){
        this->log_file << sq(object) + fg(func) + is + "Start" << std::endl;
    }
    void write_end(std::string object, std::string func){
        this->log_file << sq(object) + fg(func) + is + "End" << std::endl;
    }

private:
    // Squares the string
    std::string sq(std::string name){ return " [" + name + "] "; }
    // Figures the string
    std::string fg(std::string name){ return " {" + name + "} "; }
    std::string ev(std::string name){ return " " + name + " = "; }

    std::ofstream log_file;
    std::string is = " -> ";

};

#endif // LOG_CLASS_H
