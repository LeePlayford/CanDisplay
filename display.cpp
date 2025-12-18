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
        //lv_label_set_text(ui_AWAData, buf);
        //SetWindValue (value);
        break;

    case DataItem::TWS:
        sprintf(buf, "%.1f", std::abs(value));
        //lv_label_set_text(ui_TWSData, buf);
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
        //lv_label_set_text(ui_TWAData, buf);
        break;

    case DataItem::TWD:
        sprintf(buf, "%.f°", value);
        //lv_label_set_text(ui_TWDData, buf);
        break;

    case DataItem::BSP:
        sprintf(buf, "%.1f", value);
        //lv_label_set_text(ui_BSPData, buf);
        break;

    case DataItem::LAT:
        ConvertToDMS(value, buf, sizeof(buf), true);
        //lv_label_set_text(ui_LatitudeData, buf);
        break;

    case DataItem::LON:
        ConvertToDMS(value, buf, sizeof(buf), false);
        //lv_label_set_text(ui_LongitudeData, buf);
        break;

    case DataItem::COG:
        sprintf(buf, "%.f°", value);
        //lv_label_set_text(ui_COGData, buf);
        break;

    case DataItem::SOG:
        sprintf(buf, "%.01f", value);
        //lv_label_set_text(ui_SOGData, buf);
        break;

    case DataItem::DATA_ITEM_DEPTH:
        sprintf(buf, "%.01f", value);
        m_pUi->lineEdit_Data_Boat_Depth->setText(buf);
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
