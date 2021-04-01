#include "mainwindow.h"
#include <thread>
#include <QApplication>

int main(int argc, char *argv[]){
    std::thread serial_thread([](){
        system("..\\Serial_program\\serial");
    });
    std::thread robotino_thread([](){
        system("..\\Robot_program\\Robotino_program\\Release\\Robotino_program");
    });
    std::thread camera_thread([](){
        system("..\\Cam_program\\Cam_program\\x64\\Release\\Cam_program");
    });
    camera_thread.detach();
    robotino_thread.detach();
    serial_thread.detach();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
