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

#include <QFontDatabase>
#include <QDir>
#include <QGraphicsTextItem>
#include "qcgaugewidget.h"

#include "qdebug.h"

#include <NMEA2000_CAN.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

#include "display.h"
#include "devices.h"

void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);

void FluidLevel(const tN2kMsg &N2kMsg);
void WaterDepth(const tN2kMsg &N2kMsg);
void WindData(const tN2kMsg &N2kMsg);
void BoatSpeed(const tN2kMsg &N2kMsg);
void GPSPosition(const tN2kMsg &N2kMsg);
void GPSCogSog(const tN2kMsg &N2kMsg);
void Baro(const tN2kMsg &N2kMsg);
void Heading(const tN2kMsg &N2kMsg);
void ProductInfo(const tN2kMsg & N2kMsg);


typedef struct {
    unsigned long PGN;
    void (*Handler)(const tN2kMsg &N2kMsg);
} tNMEA2000Handler;

tNMEA2000Handler NMEA2000Handlers[]={
    {127505L,&FluidLevel},
    {127250L,&Heading},
    {128267L,&WaterDepth},
    {130306L,&WindData},
    {128259L,&BoatSpeed},
    {129029L,&GPSPosition},
    {129026L,&GPSCogSog},
    {130314L,&Baro},
    {126996L , &ProductInfo},
    {0,0}
};

static Display * s_pDisplayInstance;
static MainWindow * s_MainWindowInstance;
static devices * s_pDevices;


//------------------------------------------------------------
//
//------------------------------------------------------------
void OnN2kOpen()
{
    std::cout << "N2KOpened" << std::endl;
    NMEA2000.SendProductInformation(0x0);
}

//------------------------------------------------------------
//
//------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Set Widget Fonts and Gauge
    SetUpFonts();

    s_pDevices = new devices(ui);

    // start up the NMEA2000
    // Initialize NMEA2000
    //NMEA2000.SetForwardType(tNMEA2000_ForwardType::fwdt_ForwardAll);
    NMEA2000.SetN2kCANMsgBufSize(250);
#if defined (__x86_64)
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode , 65);
#else
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode , 55);
#endif
    NMEA2000.SetProductInformation("RPI", 101, "Can_Display", "1.0.0.1", "1.0.0.1");
    NMEA2000.SetDeviceInformation(1, 130, 204, 1);
    NMEA2000.EnableForward(false);
    NMEA2000.SetMsgHandler (HandleNMEA2000Msg);
    NMEA2000.SetOnOpen(OnN2kOpen);
    NMEA2000.SetDebugMode (tNMEA2000::tDebugMode::dm_None);
    //NMEA2000.SetForwardStream (&ForwardStream);
    if (NMEA2000.Open())
    {
        NMEA2000.SendProductInformation(0x0);

    }

    m_timer = new QTimer(this);
    m_timer->setInterval (10);
    m_timer->start();
    connect (m_timer , &QTimer::timeout , this , &MainWindow::timerexpired);
    connect (ui->tabWidget , &QTabWidget::currentChanged , this , &MainWindow::tabChanged);

    s_pDisplayInstance = new Display (ui);
    s_MainWindowInstance = this;

    // set u p the wind gauge
    SetUpWindGauge();


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
void MainWindow::tabChanged()
{
    if (ui->tabWidget->currentIndex() == 4)
    {
        SetUpWindGauge();
    }
    else if (ui->tabWidget->currentIndex() == 5)
    {
        s_pDevices->Update();
    }
}


//------------------------------------------------------------
//
//------------------------------------------------------------
void txpired()
{
    NMEA2000.ParseMessages();
}

//------------------------------------------------------------
//
//------------------------------------------------------------
void MainWindow::timerexpired()
{
    txpired();

    static uchar count = 0;
    static int pointer = 0;
    if (count++ == 0)
    {
        tN2kMsg N2kMsg;
        SetN2kPGN59904 (N2kMsg , 255 , 126996);
        NMEA2000.SendMsg(N2kMsg);
        pointer+=10;
        //mCompassNeedle->setCurrentValue(pointer%360);
    }
}

//------------------------------------------------------------
//
//------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
    delete s_pDevices;
    delete s_pDisplayInstance;
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

    // add data into a list
    s_pDevices->AddPGN(N2kMsg.Source , N2kMsg.PGN);
}

//-------------------------------------
//
//-------------------------------------
void ProductInfo(const tN2kMsg & N2kMsg)
{
    unsigned short N2kVersion;
    unsigned short ProductCode;
    int ModelIDSize = 48;
    char ModelID[48];
    int SwCodeSize = 32;
    char SwCode[32];
    int ModelVersionSize=32;
    char ModelVersion[32];
    int ModelSerialCodeSize = 32;
    char ModelSerialCode[32];
    unsigned char CertificationLevel;
    unsigned char LoadEquivalency;
    if (ParseN2kPGN126996(N2kMsg , N2kVersion , ProductCode , ModelIDSize , ModelID ,
                      SwCodeSize , SwCode , ModelVersionSize , ModelVersion ,
                          ModelSerialCodeSize , ModelSerialCode , CertificationLevel , LoadEquivalency))
    {
        // stick it in a window somwhere
        QString deviceName (ModelID);
        s_pDevices->AddDeviceName(N2kMsg.Source , deviceName);
    }
}

//-------------------------------------
//
//-------------------------------------
void Baro(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    unsigned char PressureInstance;
    tN2kPressureSource source;
    double Baro;

    if (ParseN2kPGN130314(N2kMsg,SID,PressureInstance, source, Baro) )
    {
        s_pDisplayInstance->UpdateDisplay(DataItem::BARO , Baro);
    }
}

//-------------------------------------
//
//-------------------------------------
void Heading(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    tN2kHeadingReference headingRef;
    double heading , deviation , variation;

    if (ParseN2kHeading(N2kMsg,SID,heading , deviation , variation , headingRef) && heading > 0)
    {
        s_pDisplayInstance->UpdateDisplay(DataItem::HDG , RadToDeg(heading));
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
            s_pDisplayInstance->UpdateDisplay(DataItem::DEPTH , DepthBelowTransducer);

        } else {
            char buf[32];
            sprintf(buf,"Depth %.2f m",DepthBelowTransducer);
            s_pDisplayInstance->UpdateDisplay(DataItem::DEPTH , DepthBelowTransducer);
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

        if (Latitude >= -90.f && Latitude <= 90.f)
            s_pDisplayInstance->UpdateDisplay (DataItem::LAT , (Latitude));
        if (Longitude >= -180.f && Latitude <= 180.f)
            s_pDisplayInstance->UpdateDisplay (DataItem::LON , (Longitude));

        s_pDisplayInstance->UpdateDisplay(DataItem::TIME , SecondsSinceMidnight);
        s_pDisplayInstance->UpdateDisplay(DataItem::DATE , DaysSince1970);
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
        s_pDisplayInstance->UpdateDisplay (DataItem::BSP , msToKnots(BoatSpeed));
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
        s_pDisplayInstance->UpdateDisplay (DataItem::SOG , msToKnots(SOG));
        s_pDisplayInstance->UpdateDisplay (DataItem::COG , RadToDeg(COG));
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
            s_pDisplayInstance->UpdateDisplay (DataItem::TWD , RadToDeg(WindAngle));
            break;
        case N2kWind_Magnetic:
            //ESP_LOGI(NMEA , "Wind Speed: %.2f m/s, Wind Angle: %.2f degrees (Magnetic)", WindSpeed, WindAngle);
            break;
        case N2kWind_Apparent:
        {
            //ESP_LOGI(NMEA , "Wind Speed: %.2f m/s, Wind Angle: %.2f degrees (Apparent)", WindSpeed, WindAngle);

            s_pDisplayInstance->UpdateDisplay (DataItem::AWS , msToKnots(WindSpeed));
            double AWA = RadToDeg(WindAngle);
            if (AWA > 180.0)
            {
                AWA -= 360.0; // Normalize to -180 to 180 degrees
            }
            s_pDisplayInstance->UpdateDisplay (DataItem::AWA , AWA);
            AWA += 90.f;
            if (AWA < 0.f) AWA+=360.f;
            s_MainWindowInstance->mCompassNeedle->setCurrentValue(AWA);
            break;
        }
        case N2kWind_True_boat:
        {
            s_pDisplayInstance->UpdateDisplay (DataItem::TWS , msToKnots(WindSpeed));
            double TWA = RadToDeg(WindAngle);
            if (TWA > 180.0)
            {
                TWA -= 360.0; // Normalize to -180 to 180 degrees
            }
            s_pDisplayInstance->UpdateDisplay (DataItem::TWA , TWA);
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

//----------------------------------------------
//
//----------------------------------------------
void MainWindow::SetUpWindGauge()
{
    gScene = new QGraphicsScene(this);
    ui->graphicsView_wind->setScene(gScene);

/*    QPen pen (QColor::fromRgb(255,255,255,255));
    QBrush brush (Qt::BrushStyle::SolidPattern);
    pen.setWidth(5);
    gScene->addEllipse(-150 , -150 , 300 , 300 , pen , brush);
    gScene->addText("Hello World");

    QGraphicsTextItem * text1 = new QGraphicsTextItem();
    text1->setPos (0,0);
    text1->setPlainText("Hello World");
    text1->setDefaultTextColor(Qt::white);
    gScene->addItem(text1);
*/
    // gauge try
    mCompassGauge = new QcGaugeWidget;

    mCompassGauge->addBackground(99);
    QcBackgroundItem *bkg1 = mCompassGauge->addBackground(92);
    bkg1->clearrColors();
    bkg1->addColor(0.1,Qt::black);
    bkg1->addColor(1.0,Qt::white);

    QcBackgroundItem *bkg2 = mCompassGauge->addBackground(88);
    bkg2->clearrColors();
    bkg2->addColor(0.1,Qt::white);
    bkg2->addColor(1.0,Qt::black);

    QcLabelItem * labels[12];
    for (int i = 0 ; i < 6 ; i++)
    {
        labels[i] = mCompassGauge->addLabel(80);
        labels[i]->setText(QString::number(i*30));
        labels[i]->setAngle((i*30) + 90);
        labels[i]->setColor(Qt::white);
    }
    for (int i = 0 ; i < 7 ; i++)
    {
        labels[i] = mCompassGauge->addLabel(80);
        labels[i]->setText(QString::number(i*30));
        labels[i]->setAngle(180 - ((i*30) + 90));
        labels[i]->setColor(Qt::white);
    }


    QcDegreesItem *deg = mCompassGauge->addDegrees(70);
    deg->setStep(30);
    deg->setMaxDegree(360);
    deg->setMinDegree(0);
    deg->setMinValue(0);
    deg->setMaxValue(360);

    deg->setColor(Qt::black);
    mCompassNeedle = mCompassGauge->addNeedle(60);
    mCompassNeedle->setNeedle(QcNeedleItem::TriangleNeedle);
    //mCompassNeedle->setValueRange(0,360);
    mCompassNeedle->setMaxDegree(360);
    mCompassNeedle->setMinDegree(0);
    mCompassNeedle->setMinValue(0);
    mCompassNeedle->setMaxValue(360);


    mCompassGauge->addBackground(7);
    //mCompassGauge->addGlass(88);
    gScene->addWidget(mCompassGauge);

    mSpeedGauge = new QcGaugeWidget;
    mSpeedGauge->addBackground(109);
    /*QcBackgroundItem *bkg11 = mSpeedGauge->addBackground(92);
    bkg11->clearrColors();
    bkg11->addColor(0.1,Qt::black);
    bkg11->addColor(1.0,Qt::white);*/

/*    QcBackgroundItem *bkg21 = mSpeedGauge->addBackground(88);
    bkg21->clearrColors();
    bkg21->addColor(0.1,Qt::gray);
    bkg21->addColor(1.0,Qt::darkGray);
*/
    QcArcItem * arc1 = mSpeedGauge->addArc(55);
    arc1->setColor(Qt::darkMagenta);
    mSpeedGauge->addDegrees(80)->setValueRange(0,360);
    //mSpeedGauge->addColorBand(50);
    QcBackgroundItem* bi =  mSpeedGauge->addBackground(80);
    bi->clearrColors();
    bi->addColor(80 , Qt::yellow);
    mSpeedGauge->addValues(80)->setValueRange(0,180);
    mSpeedGauge->addLabel(70)->setText("Kts");

    QcLabelItem *lab = mSpeedGauge->addLabel(40);
    lab->setText("0");
    mSpeedNeedle = mSpeedGauge->addNeedle(60);
    mSpeedNeedle->setLabel(lab);
    mSpeedNeedle->setColor(Qt::white);
    mSpeedNeedle->setValueRange(0,360);
    mSpeedGauge->addBackground(7);
    mSpeedNeedle->setCurrentValue(180);
    //mSpeedGauge->addGlass(88);

    //gScene->addWidget(mSpeedGauge);


    ui->graphicsView_wind->show();
}



//----------------------------------------------
//
//----------------------------------------------
#if defined (__x86_64)
const int DataFontSize = 64;
#else
const int DataFontSize = 96;
#endif

void MainWindow::SetUpFonts()
{
    // load fonts
    QDir dir;
    qDebug() << dir.absolutePath();

    QFont dataFont;
    QFont labelFont;
    QFont tabFont;

    int id = QFontDatabase::addApplicationFont(".fonts/NotoSans-Bold.ttf");
    if (id == 0)
    {
        QString data = QFontDatabase::applicationFontFamilies(id).at(0);
        dataFont.setFamily(data);
        dataFont.setPointSize(DataFontSize);
        labelFont.setFamily(data);
        labelFont.setPointSize(24);
        tabFont.setFamily(data);
        tabFont.setPointSize(34);
    }
    else
        qDebug() << "Font Not Found";

    ui->setupUi(this);
    qDebug () << qVersion();

    QWidgetList  list = QApplication::allWidgets();

    for (auto item : list)
    {
        if (item->objectName().contains(QString ("Data") , Qt::CaseSensitive))
        {
            item->setFont(dataFont);
        }
        else if (item->objectName().contains(QString ("Label") , Qt::CaseSensitive))
        {
            item->setFont(labelFont);
        }

    }
}


