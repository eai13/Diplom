#define MANUAL  0
#define PID     1

#define PID_X   0
#define PID_Y   1

#define REAL_TRAJECTORY 0
#define SET_TRAJECTORY  1
#define P_VALUE         2
#define I_VALUE         3
#define D_VALUE         4
#define SETPOINT_OUTPUT 5

#define INF 99999

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <map_camera.h>
#include <map_robotino.h>
#include <map_serial.h>
#include "plot_lib.h"
#include <fstream>
#include <Windows.h>

static std::ofstream PIDfile;
static uint64_t plot_time = 0;

static std::string name = "MAIN WINDOW";

// Map file connections
extern Robotino *  robotino;
extern Camera *    camera;
extern Serial *    serial;

// Plots
extern MapPlot * RealTrajectory;

static uint8_t PID_disp_flag = PID_X;

// Variables for PID control
static double Pcoef = 0;
static double Icoef = 0;
static double errorPrevX = 0;
static double errorPrevY = 0;
static double accumulatorX = 0;
static double accumulatorY = 0;
static double Dcoef = 0;
static double SatMax = INF;
static double SatMin = -INF;

// List for storaging the trajectory
std::list<std::array<double, 2>> set_trajectory;
std::list<std::array<double, 2>> set_trajectory_copy;

QTimer * PID_control_timer = new QTimer;

// Changing the controlling type tab
void MainWindow::on_tabwidget_controlmode_currentChanged(int index){
    if (index == MANUAL){
        ui->label_controlloopimage->setVisible(false);
        ui->lcdnumber_Pcoef->setVisible(false);
        ui->lcdnumber_Pcoef->display(0);
        ui->lcdnumber_Icoef->setVisible(false);
        ui->lcdnumber_Icoef->display(0);
        ui->lcdnumber_Dcoef->setVisible(false);
        ui->lcdnumber_Dcoef->display(0);

        ui->lcdnumber_setpoint->setVisible(false);
        ui->lcdnumber_setpoint->display(0);
        ui->lcdnumber_output->setVisible(false);
        ui->lcdnumber_output->display(0);
        ui->lcdnumber_feedback->setVisible(false);
        ui->lcdnumber_feedback->display(0);

        on_pushbutton_speedreset_clicked();

        RealTrajectory->Clear();
        set_trajectory.clear();
    }
    else if (index == PID){
        ui->label_controlloopimage->setVisible(true);
        ui->lcdnumber_Pcoef->setVisible(true);
        ui->lcdnumber_Pcoef->display(0);
        ui->lcdnumber_Icoef->setVisible(true);
        ui->lcdnumber_Icoef->display(0);
        ui->lcdnumber_Dcoef->setVisible(true);
        ui->lcdnumber_Dcoef->display(0);

        ui->lcdnumber_setpoint->setVisible(true);
        ui->lcdnumber_setpoint->display(0);
        ui->lcdnumber_output->setVisible(true);
        ui->lcdnumber_output->display(0);
        ui->lcdnumber_feedback->setVisible(true);
        ui->lcdnumber_feedback->display(0);

        on_pushbutton_speedreset_clicked();

        RealTrajectory->Clear();
        set_trajectory.clear();
    }
}

// PID coefs set
void MainWindow::on_pushbutton_PIDcoefsset_clicked(){
    std::string func = "on_pushbutton_PIDcoefsset_clicked";
    logfile->write_begin(name, func);
    // PID coefs
    if (ui->lineedit_Pvalue->text().length() > 0) Pcoef = ui->lineedit_Pvalue->text().toDouble();
    else Pcoef = 0;
    logfile->write_event(name, func, "P coef = ", Pcoef);

    if (ui->lineedit_Ivalue->text().length() > 0) Icoef = ui->lineedit_Ivalue->text().toDouble();
    else Icoef = 0;
    logfile->write_event(name, func, "I coef = ", Icoef);

    if (ui->lineedit_Dvalue->text().length() > 0) Dcoef = ui->lineedit_Dvalue->text().toDouble();
    else Dcoef = 0;
    logfile->write_event(name, func, "D coef = ", Dcoef);

    // Saturation values
    if ((ui->lineedit_PIDcontrolsaturationmax->text().length() > 0) && (ui->checkbox_saturationenable->checkState())) SatMax = ui->lineedit_PIDcontrolsaturationmax->text().toDouble();
    else SatMax = INF;
    logfile->write_event(name, func, "Sat Max = ", SatMax);

    if ((ui->lineedit_PIDcontrolsaturationmin->text().length() > 0) && (ui->checkbox_saturationenable->checkState())) SatMin = ui->lineedit_PIDcontrolsaturationmin->text().toDouble();
    else SatMin = -INF;
    logfile->write_event(name, func, "Sat Min = ", SatMin);
    logfile->write_end(name, func);
}

// Setting the speed
void MainWindow::on_pushbutton_speedset_clicked(){
    std::string func = "on_pushbutton_speedset_clicked";
    logfile->write_begin(name, func);

    logfile->write_event(name, func, "Speed set correct", ((ui->lineedit_speedset1->text().length() > 0) && (ui->lineedit_speedset2->text().length() > 0) && (ui->lineedit_speedset3->text().length() > 0)));
    if ((ui->lineedit_speedset1->text().length() > 0) && (ui->lineedit_speedset2->text().length() > 0) && (ui->lineedit_speedset3->text().length() > 0)){
        if (ui->combobox_speedsettype->currentText() == "Axial"){
            robotino->set_speed_cartesian(ui->lineedit_speedset1->text().toInt(), ui->lineedit_speedset2->text().toInt(), ui->lineedit_speedset3->text().toInt());
        }
        else if (ui->combobox_speedsettype->currentText() == "Local"){
            robotino->set_speed_motors(ui->lineedit_speedset1->text().toInt(), ui->lineedit_speedset2->text().toInt(), ui->lineedit_speedset3->text().toInt());
        }
    }

    logfile->write_end(name, func);
}

// Stopping the robot
void MainWindow::on_pushbutton_speedreset_clicked(){
    std::string func = "on_pushbutton_speedreset_clicked";
    logfile->write_begin(name, func);

    if (ui->combobox_speedsettype->currentText() == "Axial") robotino->set_speed_cartesian(0, 0, 0);
    else robotino->set_speed_motors(0, 0, 0);

    ui->lineedit_speedset1->setText("0");
    ui->lineedit_speedset2->setText("0");
    ui->lineedit_speedset3->setText("0");

    logfile->write_end(name, func);
}

// Trajectory Add point
void MainWindow::on_pushbutton_PIDtrajectoryadd_clicked(){
    std::string func = "on_pushbutton_PIDtrajectoryadd_clicked";
    logfile->write_begin(name, func);

    RealTrajectory->AddPoint(ui->lineedit_trajectoryx->text().toDouble(), ui->lineedit_trajectoryy->text().toDouble());
    set_trajectory.push_back(std::array<double, 2>({ui->lineedit_trajectoryx->text().toDouble(), ui->lineedit_trajectoryy->text().toDouble()}));
    logfile->write_event(name, func, "Point Added X =", ui->lineedit_trajectoryx->text().toDouble());
    logfile->write_event(name, func, "Point Added Y =", ui->lineedit_trajectoryy->text().toDouble());

    logfile->write_end(name, func);
}

// Trajectory delete the top element
void MainWindow::on_pushbutton_PIDtrajectorydelete_clicked(){
    std::string func = "on_pushbutton_PIDtrajectorydelete_clicked";
    logfile->write_begin(name, func);

    RealTrajectory->RemovePoint();
    if (set_trajectory.size() > 0) set_trajectory.pop_back();

    logfile->write_end(name, func);
}

// Trajectory clear
void MainWindow::on_pushbutton_PIDtrajectoryclear_clicked(){
    std::string func = "on_pushbutton_PIDtrajectoryclear_clicked";
    logfile->write_begin(name, func);

    RealTrajectory->Clear();
    set_trajectory.clear();

    logfile->write_end(name, func);
}

// Trajectory stop
void MainWindow::on_pushbutton_PIDtrajectorystop_clicked(){
    std::string func = "on_pushbutton_PIDtrajectorystop_clicked";
    logfile->write_begin(name, func);

    if (PID_control_timer->isActive()){
        PID_control_timer->stop();
        disconnect(PID_control_timer, SIGNAL(timeout()), this, SLOT(PID_control_func()));
    }

    if (PIDfile.is_open()) PIDfile.close();

    on_pushbutton_speedreset_clicked();
    ui->lcdnumber_Pcoef->display(0);
    ui->lcdnumber_Icoef->display(0);
    ui->lcdnumber_Dcoef->display(0);
    ui->lcdnumber_feedback->display(0);
    ui->lcdnumber_output->display(0);
    ui->lcdnumber_setpoint->display(0);

    accumulatorX = 0;
    accumulatorY = 0;

    plot_time = 0;

    ui->lineedit_PIDcontroldatafilename->setEnabled(true);
    ui->checkbox_PIDwritingenabled->setEnabled(true);
    ui->pushbutton_PIDtrajectorydelete->setEnabled(true);
    ui->pushbutton_PIDtrajectoryclear->setEnabled(true);
    ui->pushbutton_PIDtrajectorystart->setEnabled(true);
    ui->pushbutton_PIDtrajectorystop->setEnabled(false);

    logfile->write_end(name, func);
}

// Map width set
void MainWindow::on_lineedit_controlmodeplotwidth_editingFinished(){
    std::string func = "on_lineedit_controlmodeplotwidth_editingFinished";
    logfile->write_begin(name, func);
    RealTrajectory->SetSize(ui->lineedit_controlmodeplotwidth->text().toDouble());
    logfile->write_end(name, func);
}

// Map height set
void MainWindow::on_lineedit_controlmodeplotheight_editingFinished(){
    std::string func = "on_lineedit_controlmodeplotheight_editingFinished";
    logfile->write_begin(name, func);
    RealTrajectory->SetSize(-1, ui->lineedit_controlmodeplotheight->text().toDouble());
    logfile->write_end(name, func);
}

// PID Loop Choose
void MainWindow::on_combobox_PIDloopchoose_currentIndexChanged(int index){
    std::string func = "on_combobox_PIDloopchoose_currentIndexChanged";
    logfile->write_begin(name, func);
    logfile->write_event(name, func, "Displayed Loop", index);
    if (index == 0) PID_disp_flag = PID_X;
    else if (index == 1) PID_disp_flag = PID_Y;
    logfile->write_end(name, func);
}

// Start the ride for PID
void MainWindow::on_pushbutton_PIDtrajectorystart_clicked(){
    std::string func = "on_pushbutton_PIDtrajectorystart_clicked";
    logfile->write_begin(name, func);

    if (set_trajectory.size() > 0){
        if ((ui->checkbox_PIDwritingenabled->isChecked()) && (ui->lineedit_PIDcontroldatafilename->text().length() > 0)){
            PIDfile.open(ui->lineedit_PIDcontroldatafilename->text().toStdString() + ".csv");
            PIDfile << "TIME (sec);X;Setpoint X;Y;Setpoint Y;P;I;D;Saturation Min;Saturation Max;P Value X;I Value X;D value X;P Value Y;I Value Y;D Value Y;Speed X;Speed Y" << std::endl;
        }

        ui->pushbutton_PIDtrajectorystop->setEnabled(true);
        ui->pushbutton_PIDtrajectorystart->setEnabled(false);
        ui->pushbutton_PIDtrajectorydelete->setEnabled(false);
        ui->pushbutton_PIDtrajectoryclear->setEnabled(false);
        ui->lineedit_PIDcontroldatafilename->setEnabled(false);
        ui->checkbox_PIDwritingenabled->setEnabled(false);

        connect(PID_control_timer, SIGNAL(timeout()), this, SLOT(PID_control_func()));
        PID_control_timer->start(500);

        set_trajectory_copy = set_trajectory;
    }

    logfile->write_end(name, func);
}

// PID control timer function
void MainWindow::PID_control_func(){
    double PvalueX = 0;
    double DvalueX = 0;
    double PvalueY = 0;
    double DvalueY = 0;
    double SpeedX = 0;
    double SpeedY = 0;
    std::array<float, 6> position = camera->get_position();
    if (set_trajectory_copy.size() > 0){
        RealTrajectory->AddRealTrajectoryPoint(position[0], position[1]);
        if ((std::abs(set_trajectory_copy.front()[0] - position[0]) < 10) && (std::abs(set_trajectory_copy.front()[1] - position[1]) < 10)){
            set_trajectory_copy.pop_front();
            accumulatorX = 0;
            accumulatorY = 0;
        }
        else{
            PvalueX = (set_trajectory_copy.front()[0] - position[0]) * Pcoef;
            accumulatorX += (set_trajectory_copy.front()[0] - position[0]) * Icoef;
            DvalueX = Dcoef * (set_trajectory_copy.front()[0] - position[0] - errorPrevX) / 0.5;
            errorPrevX = set_trajectory_copy.front()[0] - position[0];
            SpeedX = PvalueX + accumulatorX + DvalueX;
            if (SpeedX > SatMax) SpeedX = SatMax;
            if (SpeedX < SatMin) SpeedX = SatMin;

            PvalueY = (set_trajectory_copy.front()[1] - position[1]) * Pcoef;
            accumulatorY += (set_trajectory_copy.front()[1] - position[1]) * Icoef;
            DvalueY = Dcoef * (set_trajectory_copy.front()[1] - position[1] - errorPrevY) / 0.5;
            errorPrevY = set_trajectory_copy.front()[1] - position[1];
            SpeedY = PvalueY + accumulatorY + DvalueY;
            if (SpeedY > SatMax) SpeedY = SatMax;
            if (SpeedY < SatMin) SpeedY = SatMin;

            robotino->set_speed_cartesian(SpeedX, SpeedY, 0);

            if (ui->combobox_PIDloopchoose->currentIndex() == 0){
                ui->lcdnumber_Pcoef->display(PvalueX);
                ui->lcdnumber_Icoef->display(accumulatorX);
                ui->lcdnumber_Dcoef->display(DvalueX);
                ui->lcdnumber_feedback->display(position[0]);
                ui->lcdnumber_output->display(SpeedX);
                ui->lcdnumber_setpoint->display(set_trajectory_copy.front()[0]);
            }
            else if (ui->combobox_PIDloopchoose->currentIndex() == 1){
                ui->lcdnumber_Pcoef->display(PvalueY);
                ui->lcdnumber_Icoef->display(accumulatorY);
                ui->lcdnumber_Dcoef->display(DvalueY);
                ui->lcdnumber_feedback->display(position[1]);
                ui->lcdnumber_output->display(SpeedY);
                ui->lcdnumber_setpoint->display(set_trajectory_copy.front()[1]);
            }

            if (PIDfile.is_open()){
                PIDfile << plot_time * 0.5 << ";" <<
                           position[0] << ";" <<
                           set_trajectory_copy.front()[0] << ";" <<
                           position[1] << ";" <<
                           set_trajectory_copy.front()[1] << ";" <<
                           Pcoef << ";" << Icoef << ";" << Dcoef << ";" <<
                           PvalueX << ";" << accumulatorX << ";" << DvalueX << ";" <<
                           PvalueY << ";" << accumulatorY << ";" << DvalueY << ";" <<
                           SpeedX << ";" << SpeedY << std::endl;
            }
            plot_time++;
        }
    }
    else{
        on_pushbutton_PIDtrajectorystop_clicked();
    }
}
