#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts>
#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>
#include <QTimer>

#include <windows.h>
#include <stdint.h>
#include <array>
#include <iostream>
#include <log_class.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushbutton_connect_clicked();

    void on_pushbutton_disconnect_clicked();

    void robot_data_reception(void);

    void on_pushbutton_setwritingperiod_clicked();

    void on_pushbutton_speedreset_clicked();

    void on_pushbutton_speedset_clicked();

    void on_combobox_speedsettype_currentTextChanged(const QString &arg1);

    void on_pushbutton_datawritestart_clicked();

    void on_pushbutton_datawritestop_clicked();

    void on_pushbutton_plotscaleset_clicked();

    void on_combobox_robotdataplotchoose_currentIndexChanged(int index);

    void on_lineedit_webcamnumber_returnPressed();

    void on_lineedit_comport_returnPressed();

    void on_lineedit_robotinoip_returnPressed();

    void on_lineedit_speedset1_returnPressed();

    void on_lineedit_speedset2_returnPressed();

    void on_lineedit_speedset3_returnPressed();

    void on_lineedit_writingperiod_returnPressed();

    void on_lineedit_plotscalemax_returnPressed();

    void on_lineedit_plotscalemin_returnPressed();

    void on_tab_change_button_clicked();

    void on_checkbox_autoscalemode_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
