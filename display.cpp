#include "display.h"
#include "ui_mainwindow.h"

#include <QLabel>


//---------------------------------
//
//---------------------------------
Display::Display(Ui::MainWindow * p_ui)
{
    m_pUi = p_ui;
}

//-------------------------------------
//
//-------------------------------------
QString GetCurrentDate(unsigned long DaysSince1970)
{
    time_t rawtime = DaysSince1970 * 86400; // Convert days to seconds
    struct tm * timeinfo = gmtime(&rawtime);
    mktime(timeinfo); // Normalize the time structure
    int m_currentYear = timeinfo->tm_year + 1900; // tm_year is years since 1900
    if (timeinfo->tm_year > 01)
    {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%d %b %Y", timeinfo);
        return QString(buffer);
    }
    return QString ("");
}


//-------------------------------------
//
//-------------------------------------
QString GetCurrentTime(double secondsSinceMidnight)
{
    int hours = static_cast<int>(secondsSinceMidnight / 3600);
    int minutes = static_cast<int>((secondsSinceMidnight - (hours * 3600)) / 60);
    int seconds = static_cast<int>(secondsSinceMidnight) % 60;

    /*if (m_isDst)
    {
        hours = (hours + 1) % 24; // Adjust for DST
    }*/
    return QString ("%1:%2:%3").arg (hours).arg(minutes).arg(seconds);

}


//---------------------------------
//
//---------------------------------
void Display::UpdateDisplay (DataItem dataItem, float value)
{
    char buf[32];
    // Update the display based on the data item type

    switch (dataItem)
    {
    case DataItem::AWS:
        sprintf(buf, "%.1f", value);
        UpdateWidgets ("Data_AWS" , buf);
        break;

    case DataItem::AWA:
        if (value < 0.0)
        {
            sprintf(buf, "< %.f°", std::abs(value));
        }
        else
        {
            sprintf(buf, " %.f° >", std::abs(value));
        }
        // Update the AWA label with the absolute value
        UpdateWidgets ("Data_AWA" , buf);
        //SetWindValue (value);
        break;

    case DataItem::TWS:
        sprintf(buf, "%.1f", std::abs(value));
        UpdateWidgets ("Data_TWS" , buf);
        break;

    case DataItem::TWA:
        if (value < 0.0)
        {
            sprintf(buf, "< %.f°", std::abs(value));
        }
        else
        {
            sprintf(buf, " %.f° >", std::abs(value));
        }
        // Update the AWA label with the absolute value
        UpdateWidgets ("Data_TWA" , buf);
        break;

    case DataItem::TWD:
        sprintf(buf, "%.f°", value);
        UpdateWidgets ("Data_TWD" , buf);
        break;

    case DataItem::BSP:
        sprintf(buf, "%.1f", value);
        UpdateWidgets ("Data_BSP" , buf);
        break;

    case DataItem::LAT:
        ConvertToDMS(value, buf, sizeof(buf), true);
        UpdateWidgets ("Data_Latitude" , buf);
        break;

    case DataItem::LON:
        ConvertToDMS(value, buf, sizeof(buf), false);
        UpdateWidgets ("Data_Longitude" , buf);
        break;

    case DataItem::COG:
        sprintf(buf, "%.f°", value);
        UpdateWidgets ("Data_COG" , buf);
        break;

    case DataItem::SOG:
        sprintf(buf, "%.01f", value);
        UpdateWidgets ("Data_SOG" , buf);
        break;

    case DataItem::DEPTH:
        sprintf(buf, "%.01f", value);
        UpdateWidgets ("Data_Depth" , buf);
        break;

    case DataItem::BARO:
        sprintf(buf, "%.f", value / 100.0f);
        UpdateWidgets ("Data_Baro" , buf);
        break;

    case DataItem::HDG:
        sprintf(buf, "%.f°", value);
        UpdateWidgets ("Data_HDG" , buf);
        break;

    case DataItem::TIME:
        {
            QString time = GetCurrentTime(value);
            UpdateWidgets("Data_Time" , time.toStdString().c_str());
            break;
        }

    case DataItem::DATE:
    {
        QString date = GetCurrentDate(value);
        UpdateWidgets("Data_Date" , date.toStdString().c_str());
        break;
    }

    default:
        break;
    }

}

//---------------------------------
//
//---------------------------------
void Display::UpdateWidgets (const char * dataItem, const char * buffer)
{
    QApplication * app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QList<QWidget*> list = app->allWidgets();//findChildren<QLabel*>();
    for (auto item : list)
    {
        if (item->objectName().contains (dataItem))
        {
            QLineEdit * s = qobject_cast<QLineEdit *>(item);
            if (s != nullptr)
                s->setText(buffer);
        }
    }
}


//-------------------------------------

//-------------------------------------
void Display::ConvertToDMS(float decimalDegrees, char* buffer, size_t bufferSize, bool isLatitude)
{
    char direction;
    if (isLatitude)
    {
        direction = (decimalDegrees >= 0) ? 'N' : 'S';
    }
    else
    {
        direction = (decimalDegrees >= 0) ? 'E' : 'W';
    }

    decimalDegrees = std::abs(decimalDegrees);
    int degrees = static_cast<int>(decimalDegrees);
    float fractional = decimalDegrees - degrees;
    float totalMinutes = fractional * 60.0f;
    int minutes = static_cast<int>(totalMinutes);
    float seconds = (totalMinutes - minutes) * 60.0f;

    if (isLatitude)
    {
        // Latitude degrees range from 0 to 90
        if (degrees > 90) degrees = 90;
        snprintf(buffer, bufferSize, "%02d°%02d'%04.1f %c", degrees, minutes, seconds, direction);

    }
    else
    {
        // Longitude degrees range from 0 to 180
        if (degrees > 180) degrees = 180;
        snprintf(buffer, bufferSize, "%03d°%02d'%04.1f %c", degrees, minutes, seconds, direction);

    }
}
