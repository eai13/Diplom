#include "mainwindow.h"
#include <log_class.h>
#include <thread>
#include <QApplication>

int main(int argc, char *argv[]){
    // Starting process of serial
    std::thread serial_thread([](){
        system("..\\Serial_program\\serial");
    });
    // Starting process for robotino
    std::thread robotino_thread([](){
        system("..\\Robot_program\\Robotino_program\\Release\\Robotino_program");
    });
    // Starting process for camera
    std::thread camera_thread([](){
        system("..\\Cam_program\\Cam_program\\x64\\Release\\Cam_program");
    });
    // Detaching all threads so they work on their own
    camera_thread.detach();
    robotino_thread.detach();
    serial_thread.detach();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
