#include "mainwindow.h"
#include "ui_mainwindow.h"

#define PLOT_WIDTH  641
#define PLOT_HEIGHT 380

#define DEFAULT_VALUE   2

#define GYRO_PLOT       0
#define ACCEL_PLOT      1
#define CUR_LOC_PLOT    2
#define CUR_AXIAL_PLOT  3
#define SPEED_PLOT      4
#define TICKS_PLOT      5
#define POSITION_PLOT   6
#define DELTA_POS_PLOT  8
#define ANGLE_PLOT      7
#define DELTA_ANG_PLOT  9

#include <map_camera.h>
#include <map_robotino.h>
#include <map_serial.h>
#include "plot_lib.h"
#include <fstream>
#include <Windows.h>
#include <log_class.h>

// File writing
char separation_symbol = ';';
std::ofstream data_file;

// Timers for data acquisition
QTimer * robot_data_timer = new QTimer;
QTimer * robotino_timer = new QTimer;
QTimer * camera_timer = new QTimer;

// Map files names
Robotino *  robotino;
Camera *    camera;
Serial *    serial;

// Plotting time
double plotting_time = 0;

// Current active plot
TimePlot * active_plot;

// Array of plot pointers
std::array<TimePlot *, 10> plots;

// Plots clearing function
void clear_plots(void){
    for (auto iter = plots.begin(); iter != plots.end(); iter++) (*iter)->Clear();
}

// MAIN
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){

    ui->setupUi(this);

    // Set validators
    ui->lineedit_comport->setValidator(new QIntValidator(0, 99, this));
    ui->lineedit_webcamnumber->setValidator(new QIntValidator(0, 99, this));

    // Set up the map files
    camera = new Camera;
    serial = new Serial;
    robotino = new Robotino;

    // Gyro plot setup
    std::vector<QString> names;
    names.push_back("Gyro X");
    names.push_back("Gyro Y");
    names.push_back("Gyro Z");
    plots[GYRO_PLOT] = new TimePlot("Gyro", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Accellerometer plot setup
    names.clear();
    names.push_back("Accel X");
    names.push_back("Accel Y");
    names.push_back("Accel Z");
    plots[ACCEL_PLOT] = new TimePlot("Accellerometer", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Axial current plot setup
    names.clear();
    names.push_back("Current X");
    names.push_back("Current Y");
    names.push_back("Current Z");
    plots[CUR_AXIAL_PLOT] = new TimePlot("Current Axial", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Local current plot setup
    names.clear();
    names.push_back("Current 1");
    names.push_back("Current 2");
    names.push_back("Current 3");
    names.push_back("Current Sum");
    plots[CUR_LOC_PLOT] = new TimePlot("Current Local", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Speed plot setup
    names.clear();
    names.push_back("Speed 1");
    names.push_back("Speed 2");
    names.push_back("Speed 3");
    plots[SPEED_PLOT] = new TimePlot("Speed", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Ticks plot setup
    names.clear();
    names.push_back("Ticks 1");
    names.push_back("Ticks 2");
    names.push_back("Ticks 3");
    plots[TICKS_PLOT] = new TimePlot("Ticks", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Position plot setup
    names.clear();
    names.push_back("X");
    names.push_back("Y");
    plots[POSITION_PLOT] = new TimePlot("Position", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Delta position plot setup
    names.clear();
    names.push_back("dX");
    names.push_back("dY");
    plots[DELTA_POS_PLOT] = new TimePlot("Delta Position", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Angle plot setup
    names.clear();
    names.push_back("Angle");
    plots[ANGLE_PLOT] = new TimePlot("Angle", names, -DEFAULT_VALUE, DEFAULT_VALUE, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Delta angle plot setup
    names.clear();
    names.push_back("dAngle");
    plots[DELTA_ANG_PLOT] = new TimePlot("Delta Angle", names, -1, 1, PLOT_WIDTH, PLOT_HEIGHT, ui->horizontalframe_robotdataplot);

    // Setting dAng plotting as initial
    for (auto iter = plots.begin(); iter != plots.end(); iter++) (*iter)->SetActive(false);
    plots[DELTA_ANG_PLOT]->SetActive(true);
    active_plot = plots[DELTA_ANG_PLOT];
    ui->lineedit_plotscalemax->setText(QString::fromStdString(std::to_string(active_plot->GetMax())));
    ui->lineedit_plotscalemin->setText(QString::fromStdString(std::to_string(active_plot->GetMin())));
}

// Destructor
MainWindow::~MainWindow(){
    // Stopping all the timers of active
    on_pushbutton_disconnect_clicked();
    system("taskkill /F /IM Robotino_program.exe");
    system("taskkill /F /IM serial.exe");
    system("taskkill /F /IM Cam_program.exe");
    delete ui;
}

// Timers for data reception
void MainWindow::robot_data_reception(){
    // Writing data acquisition mode for serial (gyro and accel)
    if (ui->combobox_gyroaccelprocesstype->currentText() == "True")
        serial->set_processing_type(MODE_TRUE);
    else if (ui->combobox_gyroaccelprocesstype->currentText() == "Avg")
        serial->set_processing_type(MODE_AVG);

    // Writing data acquisition mode for robotino (current, speed and encoder position)
    if (ui->combobox_currentprocesstype->currentText() == "True")
        robotino->set_current_process_type(MODE_TRUE);
    else if (ui->combobox_currentprocesstype->currentText() == "Avg")
        robotino->set_current_process_type(MODE_AVG);
    if (ui->combobox_speedpositionprocesstype->currentText() == "True")
        robotino->set_encoder_position_process_type(MODE_TRUE);
    else if (ui->combobox_speedpositionprocesstype->currentText() == "Avg")
        robotino->set_encoder_position_process_type(MODE_AVG);

    // Getting gyro and accel data
    std::array<float, 6> data_gyro_accel = serial->get_gyro_accel();
    ui->lineedit_gyrox->setText(QString::fromStdString(std::to_string(data_gyro_accel[0])));
    ui->lineedit_gyroy->setText(QString::fromStdString(std::to_string(data_gyro_accel[1])));
    ui->lineedit_gyroz->setText(QString::fromStdString(std::to_string(data_gyro_accel[2])));
    ui->lineedit_accelx->setText(QString::fromStdString(std::to_string(data_gyro_accel[3])));
    ui->lineedit_accely->setText(QString::fromStdString(std::to_string(data_gyro_accel[4])));
    ui->lineedit_accelz->setText(QString::fromStdString(std::to_string(data_gyro_accel[5])));
    // Plotting
    plots[GYRO_PLOT]->AddPoint(std::vector<float>(data_gyro_accel.begin(), data_gyro_accel.begin() + 3));
    plots[ACCEL_PLOT]->AddPoint(std::vector<float>(data_gyro_accel.begin() + 3, data_gyro_accel.begin() + 6));

    // Getting current data
    std::array<float, 7> data_current = robotino->get_current();
    ui->lineedit_currentwheel0->setText(QString::fromStdString(std::to_string(data_current[0])));
    ui->lineedit_currentwheel1->setText(QString::fromStdString(std::to_string(data_current[1])));
    ui->lineedit_currentwheel2->setText(QString::fromStdString(std::to_string(data_current[2])));
    ui->lineedit_currentsum->setText(QString::fromStdString(std::to_string(data_current[3])));
    ui->lineedit_currentx->setText(QString::fromStdString(std::to_string(data_current[4])));
    ui->lineedit_currenty->setText(QString::fromStdString(std::to_string(data_current[5])));
    ui->lineedit_currentz->setText(QString::fromStdString(std::to_string(data_current[6])));
    // Plotting
    plots[CUR_LOC_PLOT]->AddPoint(std::vector<float>(data_current.begin(), data_current.begin() + 4));
    plots[CUR_AXIAL_PLOT]->AddPoint(std::vector<float>(data_current.begin() + 4, data_current.begin() + 7));

    // Getting speed and ticks data
    std::array<float, 6> data_spd_tck = robotino->get_position();
    ui->lineedit_speedwheel0->setText(QString::fromStdString(std::to_string(data_spd_tck[0])));
    ui->lineedit_speedwheel1->setText(QString::fromStdString(std::to_string(data_spd_tck[1])));
    ui->lineedit_speedwheel2->setText(QString::fromStdString(std::to_string(data_spd_tck[2])));
    ui->lineedit_positionwheel0->setText(QString::fromStdString(std::to_string(data_spd_tck[3])));
    ui->lineedit_positionwheel1->setText(QString::fromStdString(std::to_string(data_spd_tck[4])));
    ui->lineedit_positionwheel2->setText(QString::fromStdString(std::to_string(data_spd_tck[5])));
    // Plotting
    plots[SPEED_PLOT]->AddPoint(std::vector<float>(data_spd_tck.begin(), data_spd_tck.begin() + 3));
    plots[TICKS_PLOT]->AddPoint(std::vector<float>(data_spd_tck.begin() + 3, data_spd_tck.begin() + 6));

    // Getting position data
    std::array<float, 6> data_camera = camera->get_position();
    ui->lineedit_camerax->setText(QString::fromStdString(std::to_string(data_camera[0])));
    ui->lineedit_cameray->setText(QString::fromStdString(std::to_string(data_camera[1])));
    ui->lineedit_camerafi->setText(QString::fromStdString(std::to_string(data_camera[2])));
    ui->lineedit_cameradeltax->setText(QString::fromStdString(std::to_string(data_camera[3])));
    ui->lineedit_cameradeltay->setText(QString::fromStdString(std::to_string(data_camera[4])));
    ui->lineedit_cameradeltafi->setText(QString::fromStdString(std::to_string(data_camera[5])));
    // Plotting
    plots[POSITION_PLOT]->AddPoint(std::vector<float>(data_camera.begin(), data_camera.begin() + 2));
    plots[ANGLE_PLOT]->AddPoint(std::vector<float>(data_camera.begin() + 2, data_camera.begin() + 3));
    plots[DELTA_POS_PLOT]->AddPoint(std::vector<float>(data_camera.begin() + 3, data_camera.begin() + 5));
    plots[DELTA_ANG_PLOT]->AddPoint(std::vector<float>(data_camera.begin() + 5, data_camera.begin() + 6));

    // Data writing
    if (data_file.is_open()){
        // Time
        data_file << plots[POSITION_PLOT]->GetTime() << separation_symbol;
        // Gyro X, Gyro Y, Gyro Z
        data_file << data_gyro_accel[0] << separation_symbol << data_gyro_accel[1] << separation_symbol << data_gyro_accel[2] << separation_symbol;
        // Accel X, Accel Y, Accel Z
        data_file << data_gyro_accel[3] << separation_symbol << data_gyro_accel[4] << separation_symbol << data_gyro_accel[5] << separation_symbol;
        // Current 1, Current 2, Current 3, Current Sum
        data_file << data_current[0] << separation_symbol << data_current[1] << separation_symbol << data_current[2] << separation_symbol << data_current[3] << separation_symbol;
        // Current X, Current Y, Current Z
        data_file << data_current[4] << separation_symbol << data_current[5] << separation_symbol << data_current[6] << separation_symbol;
        // Speed 1, Speed 2, Speed 3
        data_file << data_spd_tck[0] << separation_symbol << data_spd_tck[1] << separation_symbol << data_spd_tck[2] << separation_symbol;
        // Ticks 1, Ticks 2, Ticks 3
        data_file << data_spd_tck[3] << separation_symbol << data_spd_tck[4] << separation_symbol << data_spd_tck[5] << separation_symbol;
        // Position, X and Y
        data_file << data_camera[0] << separation_symbol << data_camera[1] << separation_symbol;
        // Angle
        data_file << data_camera[2] << separation_symbol;
        // Position, dX and dY
        data_file << data_camera[3] << separation_symbol << data_camera[4] << separation_symbol;
        // dAngle
        data_file << data_camera[5] << separation_symbol;
        // Process type gyro and accel
        data_file << ui->combobox_gyroaccelprocesstype->currentText().toStdString() << separation_symbol;
        // Process type current
        data_file << ui->combobox_currentprocesstype->currentText().toStdString() << separation_symbol;
        // Process type speed and ticks
        data_file << ui->combobox_speedpositionprocesstype->currentText().toStdString() << std::endl;
    }
}

// Connection button clicked
void MainWindow::on_pushbutton_connect_clicked(){
    // Reading data from the connection fields
    if ((ui->lineedit_comport->text().length() > 0) && (ui->lineedit_webcamnumber->text().length() > 0) && (ui->lineedit_robotinoip->text().length() > 0)){
        serial->set_com(ui->lineedit_comport->text().toInt(), ui->combobox_baudrate->currentText().toInt());
        robotino->set_ip(ui->lineedit_robotinoip->text().toStdString());
        camera->set_camera_number(ui->lineedit_webcamnumber->text().toInt());
    }
    else return;

    // Sending connection command
    serial->set_connection();
    robotino->set_connection();
    camera->set_connection();

    // Checking for connection availability for all channels
    std::cout << (uint16_t)serial->check_connection() << std::endl;
    if (serial->check_connection() == 0){
        ui->label_connectionserial->setText("DISCONNECTED");
        serial->disconnect();
    }
    else if (serial->check_connection() == 1) ui->label_connectionserial->setText("CONN AVAILABLE");

    std::cout << (uint16_t)robotino->check_connection() << std::endl;
    if (robotino->check_connection() == 0){
        ui->label_connectionrobotino->setText("DISCONNECTED");
        robotino->disconnect();
    }
    else if (robotino->check_connection() == 1) ui->label_connectionrobotino->setText("CONN AVAILABLE");

    std::cout << (uint16_t)camera->check_connection() << std::endl;
    if (camera->check_connection() == 0){
        ui->label_connectionwebcam->setText("DISCONNECTED");
        camera->disconnect();
    }
    else if (camera->check_connection() == 1) ui->label_connectionwebcam->setText("CONN AVAILABLE");

    // If connection succeeded
    if ((serial->check_connection() == 1) && (robotino->check_connection() == 1) && (camera->check_connection() == 1)){
        // Setting the availability for GUI elements
        ui->label_connectionserial->setText("CONNECTED");
        ui->label_connectionrobotino->setText("CONNECTED");
        ui->label_connectionwebcam->setText("CONNECTED");
        ui->lineedit_comport->setEnabled(false);
        ui->lineedit_robotinoip->setEnabled(false);
        ui->lineedit_webcamnumber->setEnabled(false);
        ui->groupbox_robotdata->setEnabled(true);
        ui->pushbutton_disconnect->setEnabled(true);
        ui->pushbutton_connect->setEnabled(false);
        ui->combobox_baudrate->setEnabled(false);

        ui->pushbutton_datawritestop->setEnabled(false);
        ui->pushbutton_datawritestart->setEnabled(true);

        ui->lineedit_writingperiod->setText("500");
        ui->lineedit_speedset1->setText("0");
        ui->lineedit_speedset2->setText("0");
        ui->lineedit_speedset3->setText("0");

        clear_plots();

        // Setting the processing type for all fields
        serial->set_processing_type(MODE_TRUE);
        robotino->set_current_process_type(MODE_TRUE);
        robotino->set_encoder_position_process_type(MODE_TRUE);

        // Setting the timers
        connect(robot_data_timer, SIGNAL(timeout()), this, SLOT(robot_data_reception()));
        robot_data_timer->start(500);
    }
    else{
        serial->disconnect();
        robotino->disconnect();
        camera->disconnect();
        return;
    }
}
// Disconnect button clicked
void MainWindow::on_pushbutton_disconnect_clicked(){
    std::cout << "DISCONNECT: "; // <<< DEBUG
    // Stopping the timer
    if (robot_data_timer->isActive()){
        std::cout << "TIMER STOPPED ; "; // <<< DEBUG
        robot_data_timer->stop();
        robot_data_timer->disconnect(robot_data_timer, SIGNAL(timeout()), this, SLOT(robot_data_reception()));
    }

    // Write file closing
    if (data_file.is_open()){
        std::cout << "DATA FILE CLOSED ; "; // <<< DEBUG
        data_file.close();
    }

    // Serial data processing
    if (serial->check_connection() == 1) {
        std::cout << "SERIAL DISCONNECT ; "; // <<< DEBUG
        serial->disconnect();
    }
    ui->lineedit_gyrox->setText(" ");
    ui->lineedit_gyroy->setText(" ");
    ui->lineedit_gyroz->setText(" ");
    ui->lineedit_accelx->setText(" ");
    ui->lineedit_accely->setText(" ");
    ui->lineedit_accelz->setText(" ");
    ui->combobox_gyroaccelprocesstype->setCurrentIndex(0);

    // Roboino data processing
    if (robotino->check_connection() == 1){
        robotino->disconnect();
        std::cout << "ROBOTINO DISCONNECT ; "; // <<< DEBUG
    }
    ui->lineedit_positionwheel0->setText(" ");
    ui->lineedit_positionwheel1->setText(" ");
    ui->lineedit_positionwheel2->setText(" ");
    ui->lineedit_speedwheel0->setText(" ");
    ui->lineedit_speedwheel1->setText(" ");
    ui->lineedit_speedwheel2->setText(" ");
    ui->lineedit_currentwheel0->setText(" ");
    ui->lineedit_currentwheel1->setText(" ");
    ui->lineedit_currentwheel2->setText(" ");
    ui->lineedit_currentx->setText(" ");
    ui->lineedit_currenty->setText(" ");
    ui->lineedit_currentz->setText(" ");
    ui->lineedit_currentsum->setText(" ");
    ui->combobox_currentprocesstype->setCurrentIndex(0);
    ui->combobox_speedpositionprocesstype->setCurrentIndex(0);

    // Camera data processing
    if (camera->check_connection() == 1){
        camera->disconnect();
        std::cout << "CAMERA DISCONNECT ; "; // <<< DEBUG
    }
    ui->lineedit_cameradeltafi->setText(" ");
    ui->lineedit_cameradeltax->setText(" ");
    ui->lineedit_cameradeltay->setText(" ");
    ui->lineedit_camerafi->setText(" ");
    ui->lineedit_camerax->setText(" ");
    ui->lineedit_cameray->setText(" ");
    Sleep(1000);

    // Setting GUI inerface elements availability
    ui->pushbutton_connect->setEnabled(true);
    ui->pushbutton_disconnect->setEnabled(false);
    ui->groupbox_robotdata->setEnabled(false);
    ui->lineedit_comport->setEnabled(true);
    ui->lineedit_robotinoip->setEnabled(true);
    ui->lineedit_webcamnumber->setEnabled(true);
    ui->combobox_baudrate->setEnabled(true);
    std::cout << std::endl; // <<< DEBUG

    ui->label_connectionrobotino->setText("DISCONNECTED");
    ui->label_connectionserial->setText("DISCONNECTED");
    ui->label_connectionwebcam->setText("DISCONNECTED");
}

void MainWindow::on_pushbutton_setwritingperiod_clicked(){
    std::cout << "WRITING PERIOD CHANGED: "; // <<< DEBUG
    if (ui->lineedit_writingperiod->text().toInt() < 20){
        ui->lineedit_writingperiod->setText("20");
        std::cout << "WAS <20 ; "; // <<< DEBUG
    }

    float dt = ui->lineedit_writingperiod->text().toFloat() / 1000;
    std::cout << "dt = " << dt << " ; "; // <<< DEBUG

    for (auto iter = plots.begin(); iter != plots.end(); iter++) (*iter)->SetTimeIncrement(dt);

    if (robot_data_timer->isActive()) robot_data_timer->stop();
    robot_data_timer->start(ui->lineedit_writingperiod->text().toInt());
    std::cout << "TIMER RESET" << std::endl; // <<< DEBUG
}

// Stopping the robot
void MainWindow::on_pushbutton_speedreset_clicked(){
    std::cout << "SPEED RESET: "; // <<< DEBUG
    if (ui->combobox_speedsettype->currentText() == "Axial"){
        std::cout << "NULLS"; // <<< DEBUG
        robotino->set_speed_cartesian(0, 0, 0);
    }
    else{
        std::cout << "NULLS"; // <<< DEBUG
        robotino->set_speed_motors(0, 0, 0);
    }
    ui->lineedit_speedset1->setText("0");
    ui->lineedit_speedset2->setText("0");
    ui->lineedit_speedset3->setText("0");
    std::cout << std::endl; // <<< DEBUG
}

// Setting the speed
void MainWindow::on_pushbutton_speedset_clicked(){
    std::cout << "SPEED SET: "; // <<< DEBUG
    if ((ui->lineedit_speedset1->text().length() > 0) && (ui->lineedit_speedset2->text().length() > 0) && (ui->lineedit_speedset3->text().length() > 0)){
        if (ui->combobox_speedsettype->currentText() == "Axial"){
            std::cout << "AXIAL"; // <<< DEBUG
            robotino->set_speed_cartesian(ui->lineedit_speedset1->text().toInt(), ui->lineedit_speedset2->text().toInt(), ui->lineedit_speedset3->text().toInt());
        }
        else if (ui->combobox_speedsettype->currentText() == "Local"){
            std::cout << "LOCAL"; // <<< DEBUG
            robotino->set_speed_motors(ui->lineedit_speedset1->text().toInt(), ui->lineedit_speedset2->text().toInt(), ui->lineedit_speedset3->text().toInt());
        }
    }
    std::cout << std::endl;
}

// Choosing the speed set type
void MainWindow::on_combobox_speedsettype_currentTextChanged(const QString &arg1){
    std::cout << "SPEED TYPE CHANGED: "; // <<< DEBUG
    if ((ui->lineedit_speedset1->text().length() > 0) && (ui->lineedit_speedset2->text().length() > 0) && (ui->lineedit_speedset3->text().length() > 0)){
        if (arg1 == "Axial"){
            std::cout << "TO AXIAL"; // <<< DEBUG
            robotino->set_speed_cartesian(ui->lineedit_speedset1->text().toInt(), ui->lineedit_speedset2->text().toInt(), ui->lineedit_speedset3->text().toInt());
            ui->NULL_label_58->setText("X:");
            ui->NULL_label_47->setText("Y:");
            ui->NULL_label_31->setText("F:");
        }
        else if (arg1 == "Local"){
            std::cout << "TO LOCAL"; // <<< DEBUG
            robotino->set_speed_motors(ui->lineedit_speedset1->text().toInt(), ui->lineedit_speedset2->text().toInt(), ui->lineedit_speedset3->text().toInt());
            ui->NULL_label_58->setText("W1:");
            ui->NULL_label_47->setText("W2:");
            ui->NULL_label_31->setText("W3:");
        }
    }
    std::cout << std::endl; // <<< DEBUG
}

// Writing init
void MainWindow::on_pushbutton_datawritestart_clicked(){
    std::cout << "WRITING INIT: ";  // <<< DEBUG
    // If the filename is not empty
    if (ui->lineedit_datafilename->text().length() > 0){
        // Creating/Opening the file to write
        data_file.open(ui->lineedit_datafilename->text().toStdString() + ".csv");

        // Writing the head of the file
        data_file << "Gyro X" << separation_symbol << "Gyro Y" << separation_symbol << "Gyro Z" << separation_symbol;
        data_file << "Accel X" << separation_symbol << "Accel Y" << separation_symbol << "Accel Z" << separation_symbol;
        data_file << "Current 1 (A)" << separation_symbol << "Current 2 (A)" << separation_symbol << "Current 3 (A)" << separation_symbol << "Current Sum (A)" << separation_symbol;
        data_file << "Current X (A)" << separation_symbol << "Current Y (A)" << separation_symbol << "Current Z (A)" << separation_symbol;
        data_file << "Speed 1 (RPM)" << separation_symbol << "Speed 2 (RPM)" << separation_symbol << "Speed 3 (RPM)" << separation_symbol;
        data_file << "Ticks 1" << separation_symbol << "Ticks 2" << separation_symbol << "Ticks 3" << separation_symbol;
        data_file << "X (PIX)" << separation_symbol << "Y (PIX)" << separation_symbol;
        data_file << "Angle (DEG)" << separation_symbol;
        data_file << "dX (MM)" << separation_symbol << "dY (MM)" << separation_symbol;
        data_file << "dAngle (DEG)" << separation_symbol;
        data_file << "Gyro/Accel TYPE" << separation_symbol << "Current TYPE" << separation_symbol << "Speed/Ticks TYPE" << std::endl;

        // Disable start button, enable stop button
        ui->pushbutton_datawritestop->setEnabled(true);
        ui->pushbutton_datawritestart->setEnabled(false);

        std::cout << "HAT READY"; // <<< DEBUG
    }
    std::cout << std::endl; // <<< DEBUG
}

void MainWindow::on_pushbutton_datawritestop_clicked(){
    ui->pushbutton_datawritestart->setEnabled(true);
    ui->pushbutton_datawritestop->setEnabled(false);
    data_file.close();
    std::cout << "WRITING STOPPED" << std::endl; // <<< DEBUG
}

// Scale changing
void MainWindow::on_pushbutton_plotscaleset_clicked(){
    if (ui->lineedit_plotscalemax->text().toInt() > ui->lineedit_plotscalemin->text().toInt()){
        active_plot->SetValueAxisRange(ui->lineedit_plotscalemin->text().toFloat(), ui->lineedit_plotscalemax->text().toFloat());
    }
}

// Choosing the plotting data
void MainWindow::on_combobox_robotdataplotchoose_currentIndexChanged(int index){
    // Show active plot
    for (auto iter = plots.begin(); iter != plots.end(); iter++) (*iter)->SetActive(false);
    plots[index]->SetActive(true);
    active_plot = plots[index];

    // Setting scale
    ui->lineedit_plotscalemax->setText(QString::fromStdString(std::to_string(active_plot->GetMax())));
    ui->lineedit_plotscalemin->setText(QString::fromStdString(std::to_string(active_plot->GetMin())));
}

void MainWindow::on_lineedit_webcamnumber_returnPressed(){
    on_pushbutton_connect_clicked();
}

void MainWindow::on_lineedit_comport_returnPressed(){
    on_pushbutton_connect_clicked();
}

void MainWindow::on_lineedit_robotinoip_returnPressed(){
    on_pushbutton_connect_clicked();
}

void MainWindow::on_lineedit_speedset1_returnPressed(){
    on_pushbutton_speedset_clicked();
}

void MainWindow::on_lineedit_speedset2_returnPressed(){
    on_pushbutton_speedset_clicked();
}

void MainWindow::on_lineedit_speedset3_returnPressed(){
    on_pushbutton_speedset_clicked();
}

void MainWindow::on_lineedit_writingperiod_returnPressed(){
    on_pushbutton_setwritingperiod_clicked();
}

void MainWindow::on_lineedit_plotscalemax_returnPressed(){
    on_pushbutton_plotscaleset_clicked();
}

void MainWindow::on_lineedit_plotscalemin_returnPressed(){
    on_pushbutton_plotscaleset_clicked();
}

void MainWindow::on_tab_change_button_clicked(){
    if (ui->tabWidget->currentIndex() == 3) ui->tabWidget->setCurrentIndex(0);
    else ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() + 1);
}

void MainWindow::on_checkbox_autoscalemode_stateChanged(int arg1){
    ui->lineedit_plotscalemin->setEnabled((arg1 == 2) ? true : false);
    ui->lineedit_plotscalemax->setEnabled((arg1 == 2) ? true : false);
    ui->pushbutton_plotscaleset->setEnabled((arg1 == 2) ? true : false);
    for (auto iter = plots.begin(); iter != plots.end(); iter++) (*iter)->AutoScale(arg1);
}
