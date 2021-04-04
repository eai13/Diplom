#include "mainwindow.h"
#include <log_class.h>
#include <thread>
#include <string.h>
#include <QApplication>

extern Log * logfile;
static std::string name = "MAIN";

int main(int argc, char *argv[]){
    std::string func = "main";
    logfile->write_begin(name, func);
    // Starting process of serial
    std::thread serial_thread([](){
        system("..\\Serial_program\\serial");
    });
    logfile->write_event(name, func, "Serial thread", 1);
    // Starting process for robotino
    std::thread robotino_thread([](){
        system("..\\Robot_program\\Robotino_program\\Release\\Robotino_program");
    });
    logfile->write_event(name, func, "Robotino thread", 1);
    // Starting process for camera
    std::thread camera_thread([](){
        system("..\\Cam_program\\Cam_program\\x64\\Release\\Cam_program");
    });
    logfile->write_event(name, func, "Camera thread", 1);
    // Detaching all threads so they work on their own
    camera_thread.detach();
    robotino_thread.detach();
    serial_thread.detach();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
