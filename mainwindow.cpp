#include "mainwindow.h"
#include "ui_mainwindow.h"

//#include "page_6_fields.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/termbits.h> /* struct termios2 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>


#include "qdebug.h"

#include <NMEA2000_CAN.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

#include "display.h"

void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);

void FluidLevel(const tN2kMsg &N2kMsg);
void WaterDepth(const tN2kMsg &N2kMsg);
void WindData(const tN2kMsg &N2kMsg);
void BoatSpeed(const tN2kMsg &N2kMsg);
void GPSPosition(const tN2kMsg &N2kMsg);
void GPSCogSog(const tN2kMsg &N2kMsg);


typedef struct {
    unsigned long PGN;
    void (*Handler)(const tN2kMsg &N2kMsg);
} tNMEA2000Handler;

tNMEA2000Handler NMEA2000Handlers[]={
    {127505L,&FluidLevel},
    {128267L,&WaterDepth},
    {130306L,&WindData},
    {128259L,&BoatSpeed},
    {129029L,&GPSPosition},
    {129026L,&GPSCogSog},
    {0,0}
};

static Display * s_pDisplayInstance;

//------------------------------------------------------------
//
//------------------------------------------------------------
void OnN2kOpen() {
    /*for ( size_t i=0; i<nN2kSendMessages; i++ ) {
    if ( N2kSendMessages[i].Scheduler.IsEnabled() ) N2kSendMessages[i].Scheduler.UpdateNextTime();
  }
  Sending=true;*/
    //ESP_LOGI(NMEA , "NMEA2000 OnN2kOpen Called **************");
    std::cout << "N2KOpened" << std::endl;
}

//------------------------------------------------------------
//
//------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug () << qVersion();

    // start up the NMEA2000
    // Initialize NMEA2000
    //NMEA2000.SetForwardType(tNMEA2000_ForwardType::fwdt_ForwardAll);
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode , 55);
    NMEA2000.SetProductInformation("ESP32", 100, "Can_Display", "1.0.0.0", "1.0.0.0");
    NMEA2000.SetDeviceInformation(1, 130, 204, 1);
    NMEA2000.EnableForward(false);
    NMEA2000.SetMsgHandler (HandleNMEA2000Msg);
    NMEA2000.SetOnOpen(OnN2kOpen);
    NMEA2000.SetDebugMode (tNMEA2000::tDebugMode::dm_None);
    //NMEA2000.SetForwardStream (&ForwardStream);
    if (NMEA2000.Open())
    {
        NMEA2000.SendProductInformation(0x0);
        //ESP_LOGI(NMEA , "NMEA2000 Opened");


        /*tN2kMsg N2kMsg;
        SetN2kSystemTime(N2kMsg,1,17555,62000);
        if (NMEA2000.SendMsg(N2kMsg)) {
            ESP_LOGI(NMEA , "N2kSystemTime Sent");
        } else {
            ESP_LOGE(NMEA , "N2kSystemTime Send Failed");
        }*/
    }

    m_timer = new QTimer(this);
    m_timer->setInterval (10);
    m_timer->start();
    connect (m_timer , &QTimer::timeout , this , &MainWindow::timerexpired);

    s_pDisplayInstance = new Display (ui);

    //ui->textEdit->setText(qVersion());

    //connect (ui->pushButton , &QPushButton::clicked , this , &MainWindow::buttonpushed);

}

//------------------------------------------------------------
//
//------------------------------------------------------------
void MainWindow::buttonpushed()
{
    //ui->textEdit->setText("Button Pushed");
    //Page_6_Fields * dlg = new Page_6_Fields(this);
    //dlg->show();

}

//------------------------------------------------------------
//
//------------------------------------------------------------
void txpired()
{
   NMEA2000.ParseMessages();
}

void MainWindow::timerexpired()
{
    txpired();
}

MainWindow::~MainWindow()
{
    delete ui;
}


//-------------------------------------
//
//-------------------------------------
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg)
{
    int iHandler;
    // Find handler
    for (iHandler = 0; NMEA2000Handlers[iHandler].PGN != 0 && !(N2kMsg.PGN == NMEA2000Handlers[iHandler].PGN); iHandler++)
        ;
    if (NMEA2000Handlers[iHandler].PGN != 0)
    {
        NMEA2000Handlers[iHandler].Handler(N2kMsg);
    }
}


//-------------------------------------
//
//-------------------------------------
void WaterDepth(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    double DepthBelowTransducer;
    double Offset;

    if (ParseN2kWaterDepth(N2kMsg,SID,DepthBelowTransducer,Offset) ) {
        if (Offset>0) {
            char buf[32];
            sprintf(buf,"Depth %.2f m",DepthBelowTransducer);
            s_pDisplayInstance->UpdateDisplay(DataItem::DATA_ITEM_DEPTH , DepthBelowTransducer);

        } else {
            char buf[32];
            sprintf(buf,"Depth %.2f m",DepthBelowTransducer);
            s_pDisplayInstance->UpdateDisplay(DataItem::DATA_ITEM_DEPTH , DepthBelowTransducer);
        }
    }
}

//-------------------------------------
//
//-------------------------------------
void GPSPosition(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;
    double Altitude;
    tN2kGNSStype GNSStype;
    tN2kGNSSmethod GNSSmethod;
    unsigned char nSatellites;
    double HDOP;
    double PDOP;
    double GeoidalSeparation;
    unsigned char nReferenceStations;
    tN2kGNSStype ReferenceStationType;
    uint16_t ReferenceSationID;
    double AgeOfCorrection;
    double Latitude;
    double Longitude;

    if (ParseN2kPGN129029 (N2kMsg , SID , DaysSince1970 , SecondsSinceMidnight ,
                          Latitude , Longitude , Altitude ,
                          GNSStype , GNSSmethod ,
                          nSatellites , HDOP , PDOP , GeoidalSeparation ,
                          nReferenceStations , ReferenceStationType , ReferenceSationID ,
                          AgeOfCorrection
                          ))
    {
        //ESP_LOGI(NMEA , "GPS Position: Lat %.6f, Lon %.6f", Latitude, Longitude);

        //UpdateDisplay (DataItem::LAT , (Latitude));
        //UpdateDisplay (DataItem::LON , (Longitude));


    }
}

//-------------------------------------
//
//-------------------------------------
void FluidLevel(const tN2kMsg &N2kMsg) {
    /*unsigned char Instance;
    tN2kFluidType FluidType;
    double Level=0;
    double Capacity=0;*/
}

//-------------------------------------
//
//-------------------------------------
void BoatSpeed(const tN2kMsg &N2kMsg)
{
    unsigned char SID;
    double BoatSpeed;
    double GroundReferenced;
    tN2kSpeedWaterReferenceType SWRT;

    if (ParseN2kPGN128259 (N2kMsg, SID, BoatSpeed , GroundReferenced, SWRT))
    {
        //UpdateDisplay (DataItem::BSP , msToKnots(BoatSpeed));
    }
}

//-------------------------------------
//
//-------------------------------------
void GPSCogSog(const tN2kMsg &N2kMsg)
{
    unsigned char SID;
    tN2kHeadingReference ref;
    double COG;
    double SOG;

    if (ParseN2kPGN129026 (N2kMsg, SID, ref , COG , SOG))
    {
        //UpdateDisplay (DataItem::SOG , msToKnots(SOG));
        //UpdateDisplay (DataItem::COG , RadToDeg(COG));
    }
}


//-------------------------------------
//
//-------------------------------------
void WindData(const tN2kMsg &N2kMsg)
{
    unsigned char SID;
    double WindSpeed;
    double WindAngle;
    tN2kWindReference WindReference;
    // Parse the N2kMsg for wind data
    if (ParseN2kPGN130306(N2kMsg , SID , WindSpeed , WindAngle , WindReference))
    {
        switch (WindReference)
        {
        case N2kWind_True_North:
            //UpdateDisplay (DataItem::TWD , RadToDeg(WindAngle));
            break;
        case N2kWind_Magnetic:
            //ESP_LOGI(NMEA , "Wind Speed: %.2f m/s, Wind Angle: %.2f degrees (Magnetic)", WindSpeed, WindAngle);
            break;
        case N2kWind_Apparent:
        {
            //ESP_LOGI(NMEA , "Wind Speed: %.2f m/s, Wind Angle: %.2f degrees (Apparent)", WindSpeed, WindAngle);

            //UpdateDisplay (DataItem::AWS , msToKnots(WindSpeed));
            double AWA = RadToDeg(WindAngle);
            if (AWA > 180.0)
            {
                AWA -= 360.0; // Normalize to -180 to 180 degrees
            }
            //UpdateDisplay (DataItem::AWA , AWA);
            break;
        }
        case N2kWind_True_boat:
        {
            //UpdateDisplay (DataItem::TWS , msToKnots(WindSpeed));
            double TWA = RadToDeg(WindAngle);
            if (TWA > 180.0)
            {
                TWA -= 360.0; // Normalize to -180 to 180 degrees
            }
            //UpdateDisplay (DataItem::TWA , TWA);
            break;
        }

            //ESP_LOGI(NMEA , "Wind Speed: %.2f m/s, Wind Angle: %.2f degrees (True Boat)", WindSpeed, WindAngle);
            break;
        case N2kWind_True_water:
            //ESP_LOGI(NMEA , "Wind Speed: %.2f m/s, Wind Angle: %.2f degrees (True Water)", WindSpeed, WindAngle);
            break;
        default:
            //ESP_LOGI(NMEA , "Wind Speed: %.2f m/s, Wind Angle: %.2f degrees (Unknown Reference)", WindSpeed, WindAngle);
            break;
        }


    }
}
