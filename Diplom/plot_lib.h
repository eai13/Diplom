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

struct TimePlot{
public:

    // Constructor
    TimePlot(QString title, std::vector<QString> series_names, int32_t range_min, int32_t range_max, uint16_t width, uint16_t height, QFrame * parent){
        // Time resetting
        this->time = 0;
        this->time_increment = 0.5;

        // Preparing the series
        this->series.resize(series_names.size());
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
        }

        this->qcv = new QChartView(this->chart);
        this->qcv->setFixedSize(width, height);
        this->qcv->setRenderHint(QPainter::Antialiasing);
        this->qcv->setParent(parent);

        this->auto_scale_on = 0;
        this->min = -2;
        this->max = 2;
    }

    // Makes the plot visible or invisible
    void SetActive(bool state){
        this->qcv->setVisible(state);
    }

    // Setting the max and min values for the time axis
    void SetTimeAxisRange(uint32_t min_time, uint32_t max_time){
        this->axis_x->setMax(max_time);
        this->axis_x->setMin(min_time);
    }

    // Setting the max and min values for the value axis
    void SetValueAxisRange(int32_t min_value, int32_t max_value){
        this->min = min_value;
        this->max = max_value;
        this->axis_y->setMin(min_value);
        this->axis_y->setMax(max_value);
    }

    // Clearing the plot
    void Clear(void){
        for (auto iter = this->series.begin(); iter != this->series.end(); iter++) (*iter)->clear();
        this->axis_x->setMin(0);
        this->axis_x->setMax(50);
        this->time = 0;
        this->time_increment = 0.5;
        this->min = -2;
        this->max = 2;
        this->SetValueAxisRange(-2, 2);
    }

    // Adding new point to the plot
    void AddPoint(std::vector<float> data){
        if (data.size() == this->series.size()){
            for (size_t i = 0; i < data.size(); i++){
                this->series[i]->append(this->time, data[i]);
                if (auto_scale_on){
                    if (this->max <= data[i]) this->max = data[i] * 1.1;
                    if (this->min >= data[i]) this->min = data[i] * 1.1;
                    this->SetValueAxisRange(this->min, this->max);
                }
            }
            this->time += time_increment;
            if (this->time >= axis_x->max()) axis_x->setMax(this->time + 1);
        }
    }

    // Setting the increment value for time
    void SetTimeIncrement(float dt){
        this->time_increment = dt;
    }

    // Gets time of plotting
    float GetTime(void){
        return this->time;
    }

    // Returns max of the value axis
    float GetMax(void){
        return this->axis_y->max();
    }

    // Returns min of the value axis
    float GetMin(void){
        return this->axis_y->min();
    }

    // Auto Scale on/off
    void AutoScale(bool mode){
        this->auto_scale_on = mode;
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
};

#endif // PLOT_LIB_H
