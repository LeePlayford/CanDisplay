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
#include <time.h>

#include <QFontDatabase>
#include <QDir>
#include <QGraphicsTextItem>
#include "qcgaugewidget.h"

#include "qdebug.h"

#include <NMEA2000_CAN.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>


#include "Parser.h"

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
void Voltage(const tN2kMsg & N2kMsg);
void Temperature(const tN2kMsg & N2kMsg);
void SeaTemperature(const tN2kMsg & N2kMsg);
void WaterTemperature(const tN2kMsg & N2kMsg);


typedef struct {
    unsigned long PGN;
    void (*Handler)(const tN2kMsg &N2kMsg);
} tNMEA2000Handler;

tNMEA2000Handler NMEA2000Handlers[]={
 /*   {127505L,&FluidLevel},
    {127250L,&Heading},
    {127508L,&Voltage},
    {128267L,&WaterDepth},
    {130306L,&WindData},
    {128259L,&BoatSpeed},
    {129029L,&GPSPosition},
    {129026L,&GPSCogSog},
    {130310L,&WaterTemperature},
    {130312L,&SeaTemperature},
    {130314L,&Baro},
    {130316L,&Temperature},
    {126996L , &ProductInfo},*/
    {0,0}
};

static MainWindow * s_MainWindowInstance;
static Parser * s_pParser;


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

    // set up the dialog
    m_pPGNDialog = new Ui::Dialog();

    //PGNDialog->setupUi(&m_dlg);
    s_pParser = new Parser(ui , m_pPGNDialog , (tNMEA2000_SocketCAN*) &NMEA2000);

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
    NMEA2000.SetConfigurationInformation("Lee Playford" , "Baro Boards" , "Developments");
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
        s_pParser->Update();
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
    if (count++ == 0)
    {
        QList<int> missing = s_pParser->GetMissingProductData();
        foreach (auto & item , missing)
        {
            tN2kMsg N2kMsg;
            SetN2kPGN59904 (N2kMsg , item , 126996);
            NMEA2000.SendMsg(N2kMsg);
            qDebug() << "Sending Product Request " << item;
            usleep(5000);
            tN2kMsg N2kMsg1;
            SetN2kPGN59904 (N2kMsg1 , item , 126998);
            NMEA2000.SendMsg(N2kMsg1);
            qDebug() << "Sending Config Request " << item;
        }
    }
}

//------------------------------------------------------------
//
//------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
    delete s_pParser;
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
    s_pParser->AddPGN(N2kMsg);
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
void Temperature(const tN2kMsg &N2kMsg)
{
    unsigned char SID;
    double ActualTemperature , setTemperature;
    unsigned char TempInstance;
    tN2kTempSource tempSource;

    if (ParseN2kPGN130316 (N2kMsg, SID , TempInstance , tempSource , ActualTemperature , setTemperature ))
    {
        //s_pDisplayInstance->UpdateDisplay (DataItem::TEMP , KelvinToC(ActualTemperature));
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
const int SingleFontSize = 292;
#else
const int DataFontSize = 112;
const int SingleFontSize = 400;
#endif

//----------------------------------------------
//
//----------------------------------------------
void MainWindow::SetUpFonts()
{
    // load fonts
    QDir dir;
    qDebug() << dir.absolutePath();

    QFont dataFont;
    QFont labelFont;
    QFont tabFont;
    QFont SingleFont;

    int id = QFontDatabase::addApplicationFont(".fonts/NotoSans-Bold.ttf");
    if (id == 0)
    {
        QString data = QFontDatabase::applicationFontFamilies(id).at(0);
        dataFont.setFamily(data);
        dataFont.setPointSize(DataFontSize);

        labelFont.setFamily(data);
        labelFont.setPointSize(20);

        tabFont.setFamily(data);
        tabFont.setPointSize(36);
        tabFont.setBold(true);

        SingleFont.setFamily(data);
        SingleFont.setPointSize(SingleFontSize);
        SingleFont.setBold(true);

    }
    else
        qDebug() << "Font Not Found";

    ui->setupUi(this);
    qDebug () << qVersion();

    ui->tabWidget->tabBar()->setStyleSheet("QTabBar::tab:selected {\
                                   color: #00ff00;\
                                   background-color: rgb(220, 138, 221);\
                                   color: rgb(0,0,0);\
                               }");

    QWidgetList  list = QApplication::allWidgets();

    for (auto item : list)
    {
        if (item->objectName().contains("groupBox"))
        {
            item->setStyleSheet("QGroupBox {border: 5px solid rgb(220, 138, 221); margin: 1px; border-radius: 10px }\
                                 QGroupBox::title { left: 10px; top: 5px }");
            item->setFont(labelFont);
        }
        if (item->objectName().contains(QString ("Data") , Qt::CaseSensitive))
        {
            item->setFont(dataFont);
            item->setEnabled(false);
        }
        else if (item->objectName().contains(QString ("Label") , Qt::CaseSensitive))
        {
            item->setFont(labelFont);
            item->setEnabled(false);
        }
        if (item->objectName().contains(QString ("Single_Data") , Qt::CaseSensitive))
        {
            item->setFont(SingleFont);
            item->setEnabled(false);
        }


    }
}


