#ifndef PLOT_LIB_H
#define PLOT_LIB_H

#include <QtCharts>
#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>
#include <QTimer>
#include <iostream>
#include <vector>
#include <array>
#include <log_class.h>

extern Log * logfile;

struct TimePlot{
public:

    // Constructor
    TimePlot(QString title, std::vector<QString> series_names, int32_t range_min, int32_t range_max, uint16_t width, uint16_t height, QFrame * parent){
        name = title.toUpper().toStdString();
        std::string func = "Constructor";
        logfile->write_begin(name, func);
        // Time resetting
        this->time = 0;
        this->time_increment = 0.5;

        // Preparing the series
        this->series.resize(series_names.size());
        logfile->write_event(name, func, "Series size", series_names.size());
        this->chart = new QChart;

        // Setting up the axes
        this->axis_x = new QValueAxis;
        this->axis_x->setTickCount(11);
        this->axis_x->setRange(0, 50);
        this->axis_x->setTitleText("Time, sec");
        this->axis_y = new QValueAxis;
        this->axis_y->setTickCount(11);
        this->axis_y->setRange(range_min, range_max);
        this->axis_y->setTitleText(title);

        // Binding the axes to the chart
        this->chart->addAxis(this->axis_x, Qt::AlignBottom);
        this->chart->addAxis(this->axis_y, Qt::AlignLeft);

        // Setting up the legend
        this->chart->legend()->setVisible(true);
        this->chart->legend()->setAlignment(Qt::AlignBottom);

        // Adding series to the chart
        for (size_t i = 0; i < series_names.size(); i++){
            this->series[i] = new QLineSeries;
            this->chart->addSeries(this->series[i]);
            this->series[i]->attachAxis(this->axis_x);
            this->series[i]->attachAxis(this->axis_y);
            this->series[i]->setName(series_names[i]);
            logfile->write_event(name, func, "Series name " + series_names[i].toStdString(), 1);
        }

        this->qcv = new QChartView(this->chart);
        this->qcv->setFixedSize(width, height);
        this->qcv->setRenderHint(QPainter::Antialiasing);
        this->qcv->setParent(parent);

        this->auto_scale_on = 0;
        logfile->write_event(name, func, "Autoscale", this->auto_scale_on);
        this->min = -2;
        logfile->write_event(name, func, "Scale Min", this->min);
        this->max = 2;
        logfile->write_event(name, func, "Scale Max", this->max);
        logfile->write_end(name, func);
    }

    // Makes the plot visible or invisible
    void SetActive(bool state){
        std::string func = "SetActive";
        logfile->write_begin(name, func);
        this->qcv->setVisible(state);
        logfile->write_event(name, func, "active", 1);
        logfile->write_end(name, func);
    }

    // Setting the max and min values for the time axis
    void SetTimeAxisRange(uint32_t min_time, uint32_t max_time){
        std::string func = "SetTimeAxisRange";
        logfile->write_begin(name, func);
        this->axis_x->setMax(max_time);
        logfile->write_event(name, func, "Max time", max_time);
        this->axis_x->setMin(min_time);
        logfile->write_event(name, func, "Min time", min_time);
        logfile->write_end(name, func);
    }

    // Setting the max and min values for the value axis
    void SetValueAxisRange(int32_t min_value, int32_t max_value){
        std::string func = "SetValueAxisRange";
        logfile->write_begin(name, func);
        this->min = min_value;
        this->max = max_value;
        this->axis_y->setMin(min_value);
        logfile->write_event(name, func, "Min value", min_value);
        this->axis_y->setMax(max_value);
        logfile->write_event(name, func, "Max value", max_value);
        logfile->write_end(name, func);
    }

    // Clearing the plot
    void Clear(void){
        std::string func = "Clear";
        logfile->write_begin(name, func);
        for (auto iter = this->series.begin(); iter != this->series.end(); iter++) (*iter)->clear();
        this->axis_x->setMin(0);
        this->axis_x->setMax(50);
        this->time = 0;
        this->time_increment = 0.5;
        this->min = -2;
        this->max = 2;
        this->SetValueAxisRange(-2, 2);
        logfile->write_end(name, func);
    }

    // Adding new point to the plot
    void AddPoint(std::vector<float> data){
        std::string func = "AddPoint";
        logfile->write_begin(name, func);
        if (data.size() == this->series.size()){
            for (size_t i = 0; i < data.size(); i++){
                this->series[i]->append(this->time, data[i]);
                logfile->write_event(name, func, "New point Time", this->time);
                logfile->write_event(name, func, "New point Value", data[i]);
                if (auto_scale_on){
                    if (this->max <= data[i]) this->max = data[i] * 1.1;
                    if (this->min >= data[i]) this->min = data[i] * 1.1;
                    this->SetValueAxisRange(this->min, this->max);
                }
            }
            this->time += time_increment;
            logfile->write_event(name, func, "Current time", this->time);
            if (this->time >= axis_x->max()) axis_x->setMax(this->time + 1);
        }
        logfile->write_end(name, func);
    }

    // Setting the increment value for time
    void SetTimeIncrement(float dt){
        std::string func = "SetTimeIncrement";
        logfile->write_begin(name, func);
        this->time_increment = dt;
        logfile->write_event(name, func, "New time increment", this->time_increment);
        logfile->write_end(name, func);
    }

    // Gets time of plotting
    float GetTime(void){
        std::string func = "GetTime";
        logfile->write_begin(name, func);
        logfile->write_event(name, func, "Current time", this->time);
        logfile->write_end(name, func);
        return this->time;
    }

    // Returns max of the value axis
    float GetMax(void){
        std::string func = "GetMax";
        logfile->write_begin(name, func);
        logfile->write_event(name, func, "Max value", this->axis_y->max());
        logfile->write_end(name, func);
        return this->axis_y->max();
    }

    // Returns min of the value axis
    float GetMin(void){
        std::string func = "GetMin";
        logfile->write_begin(name, func);
        logfile->write_event(name, func, "Min value", this->axis_y->min());
        logfile->write_end(name, func);
        return this->axis_y->min();
    }

    // Auto Scale on/off
    void AutoScale(bool mode){
        std::string func = "AutoScale";
        logfile->write_begin(name, func);
        this->auto_scale_on = mode;
        logfile->write_event(name, func, "Autoscale", this->auto_scale_on);
        logfile->write_end(name, func);
    }

    ~TimePlot(void){
        std::string func = "Destructor";
        logfile->write_begin(name, func);
        this->qcv->~QChartView();
        this->axis_x->~QValueAxis();
        this->axis_y->~QValueAxis();
        this->chart->~QChart();
        logfile->write_end(name, func);
    }

private:
    QChartView *                qcv;
    QValueAxis *                axis_x;
    QValueAxis *                axis_y;
    QChart *                    chart;
    std::vector<QLineSeries *>  series;
    float                       time;
    float                       time_increment;
    int32_t                     min, max;
    uint8_t                     auto_scale_on;
    std::string                 name;
};

struct MapPlot{
public:

    // Constructor
    MapPlot(QFrame * parent){
        this->name = "MAP PLOT";
        std::string func = "Constructor";
        logfile->write_begin(name, func);

        this->chart = new QChart;

        // Setting up the axes
        this->axis_x = new QValueAxis;
        this->axis_x->setTickCount(6);
        this->axis_x->setRange(0, 1000);
        this->axis_y = new QValueAxis;
        this->axis_y->setTickCount(6);
        this->axis_y->setRange(0, 1000);
        this->axis_y->setReverse();

        // Binding the axes to the chart
        this->chart->addAxis(this->axis_x, Qt::AlignTop);
        this->chart->addAxis(this->axis_y, Qt::AlignLeft);
        this->chart->legend()->hide();

        // Adding series to the chart
        this->series = new QLineSeries;
        this->chart->addSeries(this->series);
        this->series->attachAxis(this->axis_x);
        this->series->attachAxis(this->axis_y);

        this->series->setName("Map");

        this->red_dot = new QLineSeries;
        this->chart->addSeries(this->red_dot);
        this->red_dot->attachAxis(this->axis_x);
        this->red_dot->attachAxis(this->axis_y);
        this->red_dot->setPen(QPen(QBrush(QColor(255, 0, 0)), 10));

        this->qcv = new QChartView(this->chart);
        this->qcv->setFixedSize(350, 300);
        this->qcv->setRenderHint(QPainter::Antialiasing);
        this->qcv->setParent(parent);

        logfile->write_end(name, func);
    }

    void AddPoint(float x, float y){
        std::string func = "AddPoint";
        logfile->write_begin(name, func);
        logfile->write_event(name, func, "X", x);
        logfile->write_event(name, func, "Y", y);
        this->red_dot->clear();
        this->red_dot->append(x, y);
        this->red_dot->append(x, y);
        this->series->append(x, y);
        logfile->write_end(name, func);
    }

    void Clear(void){
        std::string func = "Clear";
        logfile->write_begin(name, func);
        this->series->clear();
        this->red_dot->clear();
        logfile->write_end(name, func);
    }

    void SetSize(int32_t width, int32_t height){
        std::string func = "SetSize";
        logfile->write_begin(name, func);
        uint32_t wid, hei;
        if (width <= 0) wid = 100;
        else wid = width;
        if (height <= 0) hei = 100;
        else hei = height;
        logfile->write_event(name, func, "Width", wid);
        logfile->write_event(name, func, "Height", hei);
        this->axis_x->setMax(wid);
        this->axis_y->setMax(hei);
        logfile->write_end(name, func);
    }

    ~MapPlot(void){
        std::string func = "Destructor";
        logfile->write_begin(name, func);
        this->qcv->~QChartView();
        this->axis_x->~QValueAxis();
        this->axis_y->~QValueAxis();
        this->chart->~QChart();
        this->series->~QLineSeries();
        logfile->write_end(name, func);
    }

private:
    QChartView *                qcv;
    QValueAxis *                axis_x;
    QValueAxis *                axis_y;
    QChart *                    chart;
    QLineSeries *               series;
    QLineSeries *               red_dot;
    std::string                 name;
};

#endif // PLOT_LIB_H
