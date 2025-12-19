#include "display.h"
#include "ui_mainwindow.h"


//---------------------------------
//
//---------------------------------
Display::Display(Ui::MainWindow * p_ui)
{
    m_pUi = p_ui;
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
        m_pUi->lineEdit_Data_Wind_AWS->setText(buf);
        break;

    case DataItem::AWA:
        if (value < 0.0)
        {
            //lv_obj_add_flag(ui_AWAStbdInd, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(ui_AWAPortInd, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            //lv_obj_add_flag(ui_AWAPortInd, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(ui_AWAStbdInd, LV_OBJ_FLAG_HIDDEN);
        }
        // Update the AWA label with the absolute value
        sprintf(buf, "%.f", std::abs(value));
        m_pUi->lineEdit_Data_Wind_AWA->setText(buf);
        //SetWindValue (value);
        break;

    case DataItem::TWS:
        sprintf(buf, "%.1f", std::abs(value));
        m_pUi->lineEdit_Data_Wind_TWS->setText(buf);
        break;

    case DataItem::TWA:
        if (value < 0.0)
        {
            //lv_obj_add_flag(ui_TWAStbdInd, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(ui_TWAPortInd, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            //lv_obj_add_flag(ui_TWAPortInd, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(ui_TWAStbdInd, LV_OBJ_FLAG_HIDDEN);
        }
        // Update the AWA label with the absolute value
        sprintf(buf, "%.f", std::abs(value));
        m_pUi->lineEdit_Data_Wind_TWA->setText(buf);
        break;

    case DataItem::TWD:
        sprintf(buf, "%.f°", value);
        m_pUi->lineEdit_Data_Wind_TWD->setText(buf);
        break;

    case DataItem::BSP:
        sprintf(buf, "%.1f", value);
        m_pUi->lineEdit_Data_Wind_BSP->setText(buf);
        break;

    case DataItem::LAT:
        ConvertToDMS(value, buf, sizeof(buf), true);
        m_pUi->lineEdit_Data_Position_Latitude->setText (buf);
        break;

    case DataItem::LON:
        ConvertToDMS(value, buf, sizeof(buf), false);
        m_pUi->lineEdit_Data_Position_Longitude->setText (buf);
        break;

    case DataItem::COG:
        sprintf(buf, "%.f°", value);
        m_pUi->lineEdit_Data_Position_COG->setText (buf);
        break;

    case DataItem::SOG:
        sprintf(buf, "%.01f", value);
        m_pUi->lineEdit_Data_Position_SOG->setText (buf);
        break;

    case DataItem::DATA_ITEM_DEPTH:
        sprintf(buf, "%.01f", value);
        m_pUi->lineEdit_Data_Boat_Depth->setText(buf);
        break;

    case DataItem::BARO:
        sprintf(buf, "%.f", value / 100.0f);
        m_pUi->lineEdit_Data_Env_Baro->setText(buf);
        break;

    default:
        break;
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
