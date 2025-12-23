#ifndef DISPLAY_H
#define DISPLAY_H

#include <QMainWindow>
#include <QObject>

#include "mainwindow.h"

enum class DataItem {
    AWS,
    AWA,
    TWS,
    TWA,
    TWD,
    BSP,
    LAT,
    LON,
    SOG,
    COG,
    DEPTH,
    BARO,
    HDG,
    DATE,
    TIME
};

class Display : public QObject
{
    Q_OBJECT

public:
    explicit Display(Ui::MainWindow * ui);
    void UpdateDisplay (DataItem dataItem, float value);

private:
    void ConvertToDMS(float decimalDegrees, char* buffer, size_t bufferSize, bool isLatitude);
    void UpdateWidgets (const char * dataItem, const char * buffer);
    Ui::MainWindow * m_pUi;
};

#endif // DISPLAY_H
