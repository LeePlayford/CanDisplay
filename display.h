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
    DATA_ITEM_WIND_SPEED,
    DATA_ITEM_DEPTH,
    DATA_ITEM_FLUID_LEVEL,
    BARO
};

class Display : public QObject
{
    Q_OBJECT
public:
    Display(Ui::MainWindow * ui);
    void UpdateDisplay (DataItem dataItem, float value);

private:
    void ConvertToDMS(float decimalDegrees, char* buffer, size_t bufferSize, bool isLatitude);
    Ui::MainWindow * m_pUi;
};

#endif // DISPLAY_H
