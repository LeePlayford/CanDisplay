#include "Parser.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_PGNData.h"
#include "display.h"

#include "qdebug.h"

#include <QDialog>
#include <QMessageBox>

#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "ProcessCancelHelper.h"

#include "NMEA2000/N2kMessagesEnumToStr.h"
#include "NMEA2000_socketCAN/NMEA2000_SocketCAN.h"

static Display * s_pDisplayInstance;


QMap<unsigned long , QString> PGN2Str {
    {59392 , QString("ISO Acknowledgement")},
    {59904 , QString("ISO Data Request")},
    {60928 , QString("ISO Address Claim")},
    {61184 , QString("Raymarine - Prop. B Msg (Lighting?)")},
    {65311 , QString("Raymarine - Magnetic Variation")},
    {65362 , QString("Raymarine - Prop. B Msg")},
    {65370 , QString("Raymarine - Prop. B Msg")},
    {126464 , QString("PGN List Tx/Rx PGN Grp Function")},
    {126720 , QString("Ono - RayMarine E70166?")},
    {126720 , QString("Ono - RayMarine E70166?")},
    {126992 , QString("System Time")},
    {126993 , QString("Heartbeat")},
    {126996 , QString("Product Information")},
    {126998 , QString("Configuration Information")},
    {127245 , QString("Rudder")},
    {127250 , QString("Vessel Heading")},
    {127258 , QString("Magnetic Variation")},
    {127508 , QString("Battery Status")},
    {128259 , QString("Speed Water Referenced")},
    {128267 , QString("Water Depth")},
    {128275 , QString("Distance Log")},
    {129025 , QString("Position, Rapid Update")},
    {129026 , QString("COG & SOG, Rapid Update")},
    {129029 , QString("GNSS Position Data")},
    {129033 , QString("Time Offset")},
    {129038 , QString("AIS Class A Position Report")},
    {129039 , QString("AIS Class B Position Report")},
    {129040 , QString("AIS Class B Extended Report")},
    {129044 , QString("Position Datum")},
    {129283 , QString("Cross Track Error")},
    {129291 , QString("Set & Drift, Rapid Update")},
    {129539 , QString("GNSS DOP")},
    {129540 , QString("GNSS Sats In View")},
    {129541 , QString("GPS Almanac")},
    {129542 , QString("GNSS Pseudo Range Noise Statistics")},
    {129793 , QString("AIS Class A UTC and Date Report")},
    {129794 , QString("AIS Class A Static and Voyage Data")},
    {129797 , QString("AIS Binary Broadcast Message")},
    {129809 , QString("AIS Static Data Report Part A")},
    {129810 , QString("AIS Static Data Report Part B")},
    {130306 , QString("Wind Data")},
    {130310 , QString("Water Temp, Baro")},
    {130312 , QString("Temperature")},
    {130314 , QString("Actual Pressure")},
    {130316 , QString("Temperature, Ext Range")},
    {130577 , QString("Direction Data")},
    {130846 , QString("Motion sensor status")},


};

//-----------------------------
//
//-----------------------------
Parser::Parser(Ui::MainWindow * ui , Ui::Dialog * dlg , tNMEA2000_SocketCAN * p_SocketCan)
    : m_DialogActive(false) , m_loggingData(false) , m_dateTimeSet(false)
{
    m_pUi = ui;
    m_pDlg = dlg;
    m_pSocketCan = p_SocketCan;
    connect (m_pUi->listWidget_Devices , &QListWidget::itemClicked , this , &Parser::UpdatePGNs);
    connect (m_pUi->pushButton_shutdown , &QPushButton::clicked , this , &Parser::on_pushButton_shutdown_clicked);
    connect (m_pUi->pushButton_save , &QPushButton::clicked , this , &Parser::on_pushButton_save_clicked);
    connect (m_pUi->pushButton_clear , &QPushButton::clicked , this , &Parser::on_pushButton_clear_clicked);
    connect (m_pUi->pushButton_logCanBus , &QPushButton::clicked , this , &Parser::on_pushButton_logdata_clicked);
    connect (m_pUi->listWidget_PGNS , &QListWidget::itemClicked , this , &Parser::PGNClicked);

    m_pDisplayDlg = new QDialog();
    m_pDlg->setupUi(m_pDisplayDlg);

    s_pDisplayInstance = new Display (ui);
    m_buffer = new char[1024]();

}

//-----------------------------
//
//-----------------------------
Parser::~Parser()
{
    delete s_pDisplayInstance;
    delete m_buffer;
}


//-----------------------------
//
//-----------------------------
void Parser::AddDeviceName (int node , QString & name)
{
    if (m_deviceList.contains(node))
    {
        m_deviceList[node].deviceName = name;
        QMutableListIterator<int> i (m_missingProductData);
        while (i.hasNext())
        {
            if ( i.next() == node) i.remove();
        }
        Update();
    }
}
//-----------------------------
//
//-----------------------------
void Parser::AddPGN (const tN2kMsg & p_rN2kMsg)
{
    int node = p_rN2kMsg.Source;
    unsigned long PGN = p_rN2kMsg.PGN;
    if (!m_deviceList.contains(node))
    {
        const tN2kMsg * data = new tN2kMsg(p_rN2kMsg);
        m_deviceList[node].PGNList[PGN] = data;
    }
    else
    {
        if (m_deviceList.contains(node))
        {
            delete m_deviceList[node].PGNList[PGN];
            const tN2kMsg * data = new tN2kMsg(p_rN2kMsg);
            m_deviceList[node].PGNList[PGN] = data;
        }
    }
    if (m_deviceList[node].deviceName == "Unknown")
    {
        if (!m_missingProductData.contains(node))
            m_missingProductData.push_back(node);
    }

    UpdateDisplay(p_rN2kMsg);

}


//-----------------------------
//
//-----------------------------
void Parser::Update()
{
    m_pUi->listWidget_Devices->clear();
    for (auto item = m_deviceList.begin(); item != m_deviceList.end() ; ++item)
    {
        QString deviceName = QString ("(%1) - %2").arg(item.key()).arg(item->deviceName);
        m_pUi->listWidget_Devices->addItem(deviceName);
        m_pUi->listWidget_Devices->update();
    }
}


//-----------------------------
//
//-----------------------------
void Parser::PGNClicked()
{
    // get the current PGN
    QString curSel = m_pUi->listWidget_PGNS->currentItem()->text();
    qDebug() << curSel;
    // the first 5 or 6 digits are the PGN
    QString pgn;

    for (int i = 0 ; i < 10 ; i++)
    {
        if (curSel[i].isDigit())
            pgn+=curSel[i];
        else
            break;
    }
    m_currentPGN = pgn.toLong();
    m_DialogActive = true;

    m_pDisplayDlg->show();

    const tN2kMsg *data = m_deviceList[m_currentNode].PGNList[m_currentPGN];
    UpdateDisplay(*data);

    m_pDisplayDlg->exec();

    m_DialogActive = false;

    qDebug() << m_currentNode << ":" << m_currentPGN;
    m_pDlg->lineEdit_PGNData_Source->clear();
    m_pDlg->lineEdit_PGNData_Dest->clear();
    m_pDlg->lineEdit_PGNData_PGN->clear();
    m_pDlg->lineEdit_PGNData_IsFast->clear();
    m_pDlg->textEdit_PGNData->clear();
    m_pDlg->lineEdit_PGNData_MsgTime->clear();
}

//-----------------------------
//
//-----------------------------
void Parser::UpdatePGNs()
{

    m_pUi->listWidget_PGNS->clear();
    QString curSel = m_pUi->listWidget_Devices->currentItem()->text();
    qDebug() << curSel;

    // need to get the node number
    bool Started = false;
    QString strNode;
    for (auto ch : curSel)
    {
        if (!Started && ch == '(')
            Started = true;
        else if (Started && ch == ')')
                break;
        else
            strNode += ch;
    }
    int node = strNode.toInt() ;

    for (auto item = m_deviceList.begin(); item != m_deviceList.end() ; ++item)
    {
        if (node == item.key())
        {
            //QList<unsigned long> sorted = std::sort

            // get the current Node
            m_currentNode = item.key();

            //qSort(item->PGNList.begin(), item->PGNList.end() , [=] (unsigned long & p1 , unsigned long & p2)->bool {return p1 < p2;});
            QMap<unsigned long , const tN2kMsg*> PGNData = item->PGNList;
            for (auto pgn = PGNData.keyValueBegin() ; pgn != PGNData.keyValueEnd() ; ++pgn )//. constKeyValueBegin(); pgn != item->PGNList.constEnd() ; pgn++ )
            {
                QString pgnName = QString::number(pgn->first);
                if (PGN2Str.contains(pgn->first))
                {
                    pgnName += " - ";
                    pgnName += PGN2Str[pgn->first];
                }
                m_pUi->listWidget_PGNS->addItem(pgnName);
            }
            break;
        }
    }
    m_pUi->listWidget_PGNS->update();
}

//--------------------------------------------
//
//--------------------------------------------
void Parser::on_pushButton_shutdown_clicked()
{
#if defined (__AARCH64EL__)
    QMessageBox mBox;
    mBox.setText("Shutdown (Yes / No)");
    mBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = mBox.exec();
    if (ret == QMessageBox::Yes)
        system("sudo shutdown -h now");
#endif

}

//--------------------------------------------
//
//--------------------------------------------
void Parser::on_pushButton_clear_clicked()
{
    m_pUi->listWidget_PGNS->clear();
    m_pUi->listWidget_Devices->clear();
    m_deviceList.clear();
    Update();
}

//--------------------------------------------
//
//--------------------------------------------
void Parser::on_pushButton_logdata_clicked()
{
#if defined (__AARCH64EL__)
    if (m_loggingData)
    {
        m_pUi->pushButton_logCanBus->setText("Log Data");
        m_processHelper.CancelProcess();
        m_loggingData = false;
    }
    else
    {
        m_pUi->pushButton_logCanBus->setText("Logging");
        m_processHelper.StartProcess("candump -l can0 &");
        m_loggingData = true;
    }
#endif
}


//--------------------------------------------
//
//--------------------------------------------
void Parser::on_pushButton_save_clicked()
{
    // write out the data to a file
    // open a file
    QString fileName = QString("/home/lee/datalogs/data%1.txt").arg(m_secondsSinceMidnight);
    std::ofstream dataFile;
    dataFile.open(fileName.toStdString() , std::ios::out | std::ios::trunc);
    if (!dataFile.is_open())
        return;
    for (auto device = m_deviceList.begin(); device != m_deviceList.end() ; ++device)
    {
        QString line = QString("Device %1:%2").arg(device.key()).arg(device->deviceName);
        dataFile << line.toStdString() << std::endl;
        for (auto pgn = device->PGNList.begin() ; pgn != device->PGNList.end() ; ++pgn )
        {
            // write it out
            if (PGN2Str.contains(pgn.key()))
                line = QString("\tPGN : %1 - %2").arg(pgn.key()).arg(PGN2Str[pgn.key()]);
            else
                line = QString("\tPGN : %1 - Unknown").arg(pgn.key());
            dataFile << line.toStdString() << std::endl;
        }
    }
    dataFile.close();
    QMessageBox mBox;
    mBox.setText("Data Saved");
    mBox.setStandardButtons(QMessageBox::Ok);
    mBox.exec();

}

//--------------------------------------------
//
//--------------------------------------------
void Parser::UpdateDisplay(const tN2kMsg & p_rN2kMsg)
{

    switch (p_rN2kMsg.PGN)
    {
        case 59904: Handle59904(p_rN2kMsg); break;      // ISO Data Request
        case 60928: Handle60928(p_rN2kMsg); break;      // ISO Address Claim
        case 126992: Handle126992(p_rN2kMsg); break;    // System Time
        case 126996: Handle126996(p_rN2kMsg); break;    // Product Info
        case 126998: Handle126998(p_rN2kMsg); break;    // Default Group Handler
        case 127250: Handle127250(p_rN2kMsg); break;    // Heading
        case 127258: Handle127258(p_rN2kMsg); break;    // Battery Status
        case 128259: Handle128259(p_rN2kMsg); break;    // Speed Water Referenced
        case 128267: Handle128267(p_rN2kMsg); break;    // Water Depth
        case 129025: Handle129025(p_rN2kMsg); break;    // Position Rapid Update
        case 129026: Handle129026(p_rN2kMsg); break;    // COG SOG Rapid Update
        case 129029: Handle129029(p_rN2kMsg); break;    // GNSS
        case 129038: Handle129038(p_rN2kMsg); break;    // Class A Position Report
        case 129039: Handle129039(p_rN2kMsg); break;    // Class B Position Report
        case 130306: Handle130306(p_rN2kMsg); break;    // Wind Data
        case 130310: Handle130310(p_rN2kMsg); break;    // Environment Data
        case 130312: Handle130312(p_rN2kMsg); break;    // Environment Data
        case 130314: Handle130314(p_rN2kMsg); break;    // Environment
        default: HandleDefault(p_rN2kMsg); break;
    }

    // if the PGN dialog is displayed, update the data
    if (m_DialogActive && p_rN2kMsg.Source == m_currentNode && p_rN2kMsg.PGN == m_currentPGN)
    {
        UpdateDialog(p_rN2kMsg);
    }

}

//--------------------------------------------
// Update PGN Dialog
//--------------------------------------------
void Parser::UpdateDialog(const tN2kMsg & p_rN2kMsg)
{
    m_pDlg->lineEdit_PGNData_Source->setText(QString::number(p_rN2kMsg.Source));
    m_pDlg->lineEdit_PGNData_Dest->setText(QString::number(p_rN2kMsg.Destination));
    unsigned long PGN = p_rN2kMsg.PGN;
    m_pDlg->lineEdit_PGNData_PGN->setText(QString::number(PGN));
    m_pDlg->lineEdit_PGNData_MsgTime->setText(QString::number(p_rN2kMsg.MsgTime));

    if (m_pSocketCan->IsFastPacket(PGN))
        m_pDlg->lineEdit_PGNData_IsFast->setText("Yes");
    else
        m_pDlg->lineEdit_PGNData_IsFast->setText("No");


    m_pDlg->textEdit_PGNData->setText(QString(m_buffer));
    //usleep (50000);

}

//--------------------------------------------
// ISO Data Request 59904
//--------------------------------------------
void Parser::Handle59904(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    uint32_t PGN = p_rN2kMsg.Get4ByteUInt(offset);
    PGN &= 0xffffff;
    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
                     "ISO Request\n\n \
    PGN\t%d\n ",
                        PGN );
    }
}

//--------------------------------------------
// ISO Address Claim 60928
//--------------------------------------------
void Parser::Handle60928(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    uint32_t data1 = p_rN2kMsg.Get4ByteUInt(offset);
    uint8_t data2 = p_rN2kMsg.GetByte(offset);
    uint8_t data3 = p_rN2kMsg.GetByte(offset);
    uint8_t data4 = p_rN2kMsg.GetByte(offset);
    uint8_t data5 = p_rN2kMsg.GetByte(offset);

    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
                     "ISO Address Claim\n\n \
    Unique ID\t\t%d\n \
    Manuf Code\t\t%d\n \
    Device Instance\t%d\n \
    Device Function\t%d\n \
    Device Class\t\t%d\n \
    System Instance\t%d\n \
    Industry Code\t\t%d\n ",
                        data1 & 0x1fffff , (data1 >> 21)&0x3ff , data2&0x7 , (data2 >> 5)& 0x1f,
                        data3 , (data4>>1),data5&0xf);
    }
}

//--------------------------------------------
// System Time 126992
//--------------------------------------------
void Parser::Handle126992(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    uint8_t SID = p_rN2kMsg.GetByte(offset);
    uint8_t TimeSource = p_rN2kMsg.GetByte(offset)&0xf;
    uint16_t Date = p_rN2kMsg.Get2ByteUInt(offset);
    uint32_t Time = p_rN2kMsg.Get4ByteUInt(offset);

    if (m_DialogActive)
    {
        const char * TimeSourcestr = N2kEnumTypeToStr(static_cast<tN2kTimeSource>(TimeSource));
        std::sprintf (m_buffer ,
                     "System Time\n\n \
    SID\t%d\n \
    Source\t%s\n \
    Date\t%d\n \
    Time\t%d\n ",
                SID , TimeSourcestr , Date , Time );
    }
}

//-------------------------------------
// Product Info 126996
//-------------------------------------
void Parser::Handle126996(const tN2kMsg & p_rN2kMsg)
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
    if (ParseN2kPGN126996(p_rN2kMsg , N2kVersion , ProductCode , ModelIDSize , ModelID ,
                          SwCodeSize , SwCode , ModelVersionSize , ModelVersion ,
                          ModelSerialCodeSize , ModelSerialCode , CertificationLevel , LoadEquivalency))
    {
        // stick it in a window somewhere
        QString deviceName (ModelID);
        AddDeviceName(p_rN2kMsg.Source , deviceName);
    }

    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
                     "Product Info\n\n \
    Version\t\t%d\n \
    Product Code\t%d\n \
    ModelID\t\t%s\n \
    Software Code\t%s\n \
    Model Version\t%s\n \
    Model Serial Code\t%s\n \
    Cert Level\t\t%d\n \
    Load Equiv\t%d " ,
            N2kVersion , ProductCode , ModelID , SwCode , ModelVersion , ModelSerialCode , CertificationLevel , LoadEquivalency);
    }
}

//--------------------------------------------
// Default Group Handler 126998
//--------------------------------------------
void Parser::Handle126998(const tN2kMsg & p_rN2kMsg)
{
    if (m_DialogActive)
    {

        int offset = 0;
        #define MAX_FIELD_SIZE 71
        uint8_t count1 = p_rN2kMsg.GetByte(offset);
        uint8_t control = p_rN2kMsg.GetByte(offset);
        char Field1[MAX_FIELD_SIZE]={0};
        // grab count number of ascii chars
        if (control == 1 && count1 < MAX_FIELD_SIZE)
        {
            p_rN2kMsg.GetBuf(Field1 , count1-2 , offset);
        }
        uint8_t count2 = p_rN2kMsg.GetByte(offset);
        control = p_rN2kMsg.GetByte(offset);
        char Field2[MAX_FIELD_SIZE]={0};
        if (control == 1 && count2 < MAX_FIELD_SIZE)
        {
            p_rN2kMsg.GetBuf(Field2 , count2-2 , offset);
        }
        uint8_t count3 = p_rN2kMsg.GetByte(offset);
        control = p_rN2kMsg.GetByte(offset);
        char Field3[MAX_FIELD_SIZE]={0};
        if (control == 1 && count3 < MAX_FIELD_SIZE)
        {
            p_rN2kMsg.GetBuf(Field3 , count3-2 , offset);
        }

        std::sprintf (m_buffer ,
                     "Configuration Information\n\n \
                             Field 1\t%s\n \
                             Field 2\t%s\n \
                             Field 3\t%s",
                             Field1 , Field2 , Field3);
    }
}

//--------------------------------------------
// Boat Heading 127250
//--------------------------------------------
void Parser::Handle127250(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    int8_t sequence = p_rN2kMsg.GetByte(offset);
    double heading = p_rN2kMsg.Get2ByteUDouble(1e-4 , offset);
    double deviation = p_rN2kMsg.Get2ByteDouble(1e-4 , offset);
    double variation = p_rN2kMsg.Get2ByteDouble(1e-4 , offset);
    uint8_t hdgReference = p_rN2kMsg.GetByte(offset);

    if (N2kIsNA(deviation)) deviation = 0.0;
    if (N2kIsNA(variation)) variation = 0.0;

    s_pDisplayInstance->UpdateDisplay(DataItem::HDG , RadToDeg(heading));

    if (m_DialogActive)
    {
        const char * hdgRefStr = N2kEnumTypeToStr(static_cast<tN2kHeadingReference>(hdgReference&0x3));
        std::sprintf (m_buffer ,
                "Heading Data\n\n \
    Seq\t\t%d\n \
    Heading\t%.01f\n \
    Deviation\t%.01f\n \
    Variation\t%.01f\n \
    Heading Ref\t%s",
                sequence,RadToDeg(heading), RadToDeg(deviation), RadToDeg(variation), hdgRefStr);
    }
}

//-------------------------------------
// Battery Status 127258
//-------------------------------------
void Parser::Handle127258(const tN2kMsg &p_rN2kMsg)
{
    unsigned char SID;
    double ActualVoltage , ActualCurrent , ActualTemp;
    unsigned char BatInstance;

    if (ParseN2kPGN127508 (p_rN2kMsg, BatInstance ,  ActualVoltage , ActualCurrent , ActualTemp , SID))
    {
        s_pDisplayInstance->UpdateDisplay (DataItem::VOLT , ActualVoltage);
    }

    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
                     "Battery Status\n\n \
    Instance\t%d\n \
    Voltage\t%.01f\n \
    Current\t%.01f\n \
    Temperature\t%.01f\n \
    SID\t%d",
                 BatInstance,ActualVoltage, ActualCurrent, ActualTemp, SID);

    }
}

//-------------------------------------
// Boat Speed 128259
//-------------------------------------
void Parser::Handle128259(const tN2kMsg &p_rN2kMsg)
{
    unsigned char SID;
    double BoatSpeed;
    double GroundReferenced;
    tN2kSpeedWaterReferenceType SWRT;

    if (ParseN2kPGN128259 (p_rN2kMsg, SID, BoatSpeed , GroundReferenced, SWRT))
    {
        s_pDisplayInstance->UpdateDisplay (DataItem::BSP , msToKnots(BoatSpeed));
    }


    if (m_DialogActive)
    {
        const char * SWRTStr = N2kEnumTypeToStr(static_cast<tN2kSpeedWaterReferenceType>(SWRT));
        std::sprintf (m_buffer ,
                     "Speed Water Referenced\n\n \
                 Sequence\t%d\n \
                 Speed Water\t%.01fkts\n \
                 Speed Ground\t%.01f\n \
                 Speed Water Ref\t%s",
                 SID,BoatSpeed,GroundReferenced,SWRTStr);

    }
}

//-------------------------------------
// Water Depth 128267
//-------------------------------------
void Parser::Handle128267(const tN2kMsg &N2kMsg)
{
    unsigned char SID;
    double DepthBelowTransducer;
    double Offset, Range;

    if (ParseN2kPGN128267(N2kMsg,SID,DepthBelowTransducer,Offset,Range) )
    {
        if (Offset>0)
        {
            s_pDisplayInstance->UpdateDisplay(DataItem::DEPTH , DepthBelowTransducer);
        }
        else
        {
            s_pDisplayInstance->UpdateDisplay(DataItem::DEPTH , DepthBelowTransducer);
        }
    }
    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
                     "Water Depth\n\n \
    Sequence\t\t%d\n \
    Depth Below Transducer\t%.01fm\n \
    Offset\t\t\t%.01fm\n ",
                 SID,DepthBelowTransducer,Offset);
    }
}

//-------------------------------------
// Position Rapid Update 129025
//-------------------------------------
void Parser::Handle129025(const tN2kMsg &N2kMsg)
{
    double latitude;
    double longitude;

    if (ParseN2kPGN129025 (N2kMsg, latitude,longitude))
    {
        if (latitude >= -90.f && latitude <= 90.f)
            s_pDisplayInstance->UpdateDisplay (DataItem::LAT , (latitude));
        if (longitude >= -180.f && latitude <= 180.f)
            s_pDisplayInstance->UpdateDisplay (DataItem::LON , (longitude));
    }
    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
                     "Position Rapid Update\n\n \
    Latitude\t%04f\n \
    Longitude\t%04f\n ",
                         latitude,longitude);
    }
}

//-------------------------------------
// COG SOG 129026
//-------------------------------------
void Parser::Handle129026(const tN2kMsg &N2kMsg)
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
    if (m_DialogActive)
    {
        const char * typeReference = N2kEnumTypeToStr(static_cast<tN2kHeadingReference>(ref&0x3));
        std::sprintf (m_buffer ,
                     "Cog Sog\n\n \
    Seq\t\t%d\n \
    COG\t\t%.01f\n \
    SOG\t\t%.01f\n \
    Heading Ref\t%s",
                         SID,RadToDeg(COG), msToKnots(SOG), typeReference);
    }
}

//--------------------------------------------
// GNSS Position 129029
//--------------------------------------------
void Parser::Handle129029(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    int sequence = p_rN2kMsg.GetByte(offset);
    unsigned int dayCount = p_rN2kMsg.Get2ByteUInt(offset);
    double timeOfDay = p_rN2kMsg.Get4ByteUDouble(0.0001, offset);
    double latitude = p_rN2kMsg.Get8ByteDouble(1e-16, offset);
    double longitude = p_rN2kMsg.Get8ByteDouble(1e-16, offset);
    double altitude = p_rN2kMsg.Get8ByteDouble(1e-6, offset);
    int8_t typeRef = p_rN2kMsg.GetByte( offset);
    const char * typeReference = N2kEnumTypeToStr(static_cast<tN2kGNSStype>(typeRef&0xf));
    const char * methodReference = N2kEnumTypeToStr(static_cast<tN2kGNSSmethod>((typeRef >> 4) & 0x0f));
    int8_t integRef = p_rN2kMsg.GetByte(offset);
    const char * integrityReference = "Intgrity";//N2kEnumTypeToStr(static_cast<tN2kGNSS p_rN2kMsg.GetGNSSIntegrityReference((typeRef >> 4) & 0x0f);
    int8_t svCount = p_rN2kMsg.GetByte(offset);
    double HDOP = p_rN2kMsg.Get2ByteDouble(1e-2 , offset);
    double PDOP = p_rN2kMsg.Get2ByteDouble(1e-2 , offset);
    double separation = p_rN2kMsg.Get4ByteUDouble(0.01, offset);
    int refStations = p_rN2kMsg.GetByte(offset);
    int refType1 = p_rN2kMsg.Get2ByteUInt( offset);
    const char * refType1Str = "RefTypeStr";//p_rN2kMsg.GetGNSSStationReference(refType1 & 0x04);
    int refStationId1 = (refType1 >> 4) & 0x3ff;

    int refType2 = p_rN2kMsg.Get2ByteUInt(offset);
    const char * refType2Str = "RefTypeStr2";//p_rN2kMsg.GetGNSSStationReference(refType2 & 0x04);
    int refStationId2 = (refType2 >> 4) & 0x3ff;
    int ageOfDGNSS = p_rN2kMsg.Get2ByteUInt(offset);

    double tm = timeOfDay;
    int hours = (int)timeOfDay / 3600;
    tm -= hours * 3600;
    int mins = (int)tm / 60;
    tm -= mins * 60;
    int secs = (int)timeOfDay % 60;


    if (latitude >= -90.f && latitude <= 90.f)
        s_pDisplayInstance->UpdateDisplay (DataItem::LAT , (latitude));
    if (longitude >= -180.f && latitude <= 180.f)
        s_pDisplayInstance->UpdateDisplay (DataItem::LON , (longitude));

    s_pDisplayInstance->UpdateDisplay(DataItem::TIME , timeOfDay);
    s_pDisplayInstance->UpdateDisplay(DataItem::DATE , dayCount);

    SetSeconds(timeOfDay);

    // set the time and date, but only once
    //#if defined (__aarch64__)
    if (!IsDateTimeSet())
    {
        time_t rawtime = ((int)dayCount) * 86400; // Convert days to seconds
        struct tm * timeinfo = gmtime(&rawtime);
        mktime(timeinfo); // Normalize the time structure
        if (timeinfo->tm_year >= 126)
        {
            timeinfo->tm_hour = static_cast<int>(timeOfDay / 3600);
            timeinfo->tm_min = static_cast<int>((timeOfDay - (timeinfo->tm_hour * 3600)) / 60);
            timeinfo->tm_sec = static_cast<int>(timeOfDay) % 60;
            time_t time = mktime(timeinfo);
            if (time != (time_t) -1)
                ctime(&time);
            QString dateTime = QString("sudo date -s \'%1-%2-%3 %4:%5:%6\'")
                                   .arg(timeinfo->tm_year+1900)
                                   .arg(timeinfo->tm_mon+1 , 2 ,10, QLatin1Char('0'))
                                   .arg(timeinfo->tm_mday, 2 ,10, QLatin1Char('0'))
                                   .arg(timeinfo->tm_hour, 2 ,10, QLatin1Char('0'))
                                   .arg(timeinfo->tm_min, 2 ,10, QLatin1Char('0'))
                                   .arg(timeinfo->tm_sec, 2 ,10, QLatin1Char('0'));

            qDebug() << "Setting Time " << dateTime;
            SetDateTime();
#if defined (__aarch64__)
            system (dateTime.toStdString().c_str());
#endif
        }
    }

    if (m_DialogActive)
    {
        std::sprintf(m_buffer , "GNSS Position Data\n \
    Sequence\t%d\n \
    DayCount\t%d\n \
    Time\t%02d:%02d:%02d\n \
    Latitude\t%05f\n \
    Longitude\t%05f\n \
    Altitude\t%02f\n \
    Type of System\t%s\n \
    Method\t\t%s\n \
    Integrity\t%s\n \
    SV Count\t%d\n \
    HDOP\t\t%02f PDOP\t\t%02f\n \
    Separation\t%04f\n \
    Ref Stations\t%d\n \
    Ref Type 1\t%s Ref Type 1\t%d\n \
    Ref Type 2\t%s Ref Type 2\t%d\n \
    Age of DGNSS\t%d\n" ,
          sequence, dayCount, hours , mins ,
          secs ,latitude , longitude , altitude , typeReference ,
          methodReference , integrityReference , svCount , HDOP , PDOP ,
          separation , refStations, refType1Str , refStationId1 , refType2Str ,
          refStationId2 , ageOfDGNSS);
    }
}

//--------------------------------------------
// Class A Position Report 129038
//--------------------------------------------
void Parser::Handle129038(const tN2kMsg & p_rN2kMsg)
{
    if (m_DialogActive)
    {
        int offset = 0;

        // Byte 1
        uint8_t MsgId = p_rN2kMsg.GetByte(offset);
        uint8_t RepeatInd = (MsgId >> 6) & 0x3;
        MsgId &= 0x3f;

        uint32_t GenericId = p_rN2kMsg.Get4ByteUInt(offset);
        double Longitude = p_rN2kMsg.Get4ByteDouble(1e-7 , offset ,0.f);
        double Latitude = p_rN2kMsg.Get4ByteDouble(1e-7 , offset , 0.f);
        uint8_t tmp = p_rN2kMsg.GetByte(offset);
        uint8_t Accuracy = tmp & 0x1;
        uint8_t RAIM = (tmp >> 1)&0x01;
        uint8_t TimeStamp = (tmp >> 1)&0x3f;
        double COG = p_rN2kMsg.Get2ByteUDouble(1e-4 , offset , 0.f);
        double SOG = p_rN2kMsg.Get2ByteUDouble(1e-2 , offset , 0.f);
        uint32_t info = p_rN2kMsg.Get3ByteUInt(offset);
        uint32_t commState = info & 0x7ffff; // 19 bits
        uint32_t AISinfo = (info >> 21)&0x1f; // 5 bits
        double Heading = p_rN2kMsg.Get2ByteUDouble(1e-4 , offset, 0.f);
        double ROT = p_rN2kMsg.Get2ByteUDouble(1e-3 , offset , 0.f);
        tmp = p_rN2kMsg.GetByte(offset);
        uint8_t NavStatus = tmp & 0x4;

        std::sprintf (m_buffer , "Class A AIS Position Report\n\n \
    Message Id\t%d\n \
    Repeat Indicator\t%d\n \
    Generic ID\t%d\n \
    Longitude\t%0.4f\n \
    Latitude\t%0.4f\n \
    Accuracy\t%d\n \
    RAIM\t\t%d\n \
    Timestamp\t%d\n \
    COG\t\t%0.1f\n \
    SOG\t\t%0.1f\n \
    Comm State\t%d\n \
    AIS Info\t\t%d\n \
    Heading\t%0.1f\n \
    Rate Of Turn\t%0.1f\n \
    Nav Status\t%d\n" ,
                    MsgId , RepeatInd , GenericId , Longitude , Latitude,
                    Accuracy , RAIM , TimeStamp , RadToDeg(COG) , msToKnots(SOG) , commState ,
                    AISinfo , RadToDeg(Heading) , (ROT) , NavStatus);

    }
}


//--------------------------------------------
// Class B Position Report 129039
//--------------------------------------------
void Parser::Handle129039(const tN2kMsg & p_rN2kMsg)
{
    if (m_DialogActive)
    {
        int offset = 0;

        // Byte 1
        uint8_t MsgId = p_rN2kMsg.GetByte(offset);
        uint8_t RepeatInd = (MsgId >> 6) & 0x3;
        MsgId &= 0x3f;

        uint32_t GenericId = p_rN2kMsg.Get4ByteUInt(offset);
        double Longitude = p_rN2kMsg.Get4ByteDouble(1e-7 , offset ,0.f);
        double Latitude = p_rN2kMsg.Get4ByteDouble(1e-7 , offset , 0.f);
        uint8_t tmp = p_rN2kMsg.GetByte(offset);
        uint8_t Accuracy = tmp & 0x1;
        uint8_t RAIM = (tmp >> 1)&0x01;
        uint8_t TimeStamp = (tmp >> 1)&0x3f;
        double COG = p_rN2kMsg.Get2ByteUDouble(1e-4 , offset , 0.f);
        double SOG = p_rN2kMsg.Get2ByteUDouble(1e-2 , offset , 0.f);
        uint32_t info = p_rN2kMsg.Get3ByteUInt(offset);
        uint32_t commState = info & 0x7ffff; // 19 bits
        uint32_t AISinfo = (info >> 21)&0x1f; // 5 bits
        double Heading = p_rN2kMsg.Get2ByteUDouble(1e-4 , offset, 0.f);
        double ROT = p_rN2kMsg.Get2ByteUDouble(1e-3 , offset , 0.f);
        tmp = p_rN2kMsg.GetByte(offset);
        uint8_t NavStatus = tmp & 0x4;

        std::sprintf (m_buffer , "Class B AIS Position Report\n\n \
    Message Id\t%d\n \
    Repeat Indicator\t%d\n \
    Generic ID\t%d\n \
    Longitude\t%0.4f\n \
    Latitude\t%0.4f\n \
    Accuracy\t%d\n \
    RAIM\t\t%d\n \
    Timestamp\t%d\n \
    COG\t\t%0.1f\n \
    SOG\t\t%0.1f\n \
    Comm State\t%d\n \
    AIS Info\t\t%d\n \
    Heading\t%0.1f\n \
    Rate Of Turn\t%0.1f\n \
    Nav Status\t%d\n" ,
                                   MsgId , RepeatInd , GenericId , Longitude , Latitude,
                     Accuracy , RAIM , TimeStamp , RadToDeg(COG) , msToKnots(SOG) , commState ,
                     AISinfo , RadToDeg(Heading) , (ROT) , NavStatus);

    }
}

//--------------------------------------------
// Wind Data 130306
//--------------------------------------------
void Parser::Handle130306(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    int8_t sequence = p_rN2kMsg.GetByte(offset);
    double windSpeed = p_rN2kMsg.Get2ByteUDouble(1e-2 , offset);
    double windDirection = p_rN2kMsg.Get2ByteUDouble(1e-4,offset);
    uint8_t windReference = p_rN2kMsg.GetByte(offset)&0x7;

    const char * windRefStr = "Unknown";

    switch (windReference)
    {
    case N2kWind_True_North:
        windRefStr="True North";
        s_pDisplayInstance->UpdateDisplay (DataItem::TWD , RadToDeg(windDirection));
        break;

    case N2kWind_Magnetic:
        windRefStr="Magnetic North";
        break;

    case N2kWind_Apparent:
    {
        windRefStr="Apparent";
        s_pDisplayInstance->UpdateDisplay (DataItem::AWS , msToKnots(windSpeed));
        double AWA = RadToDeg(windDirection);
        if (AWA > 180.0)
        {
            AWA -= 360.0; // Normalize to -180 to 180 degrees
        }
        s_pDisplayInstance->UpdateDisplay (DataItem::AWA , AWA);
        AWA += 90.f;
        //if (AWA < 0.f) AWA+=360.f;
        //s_MainWindowInstance->mCompassNeedle->setCurrentValue(AWA);
        break;
    }
    case N2kWind_True_boat:
    {
        windRefStr="True Boat";
        s_pDisplayInstance->UpdateDisplay (DataItem::TWS , msToKnots(windSpeed));
        double TWA = RadToDeg(windDirection);
        if (TWA > 180.0)
        {
            TWA -= 360.0; // Normalize to -180 to 180 degrees
        }
        s_pDisplayInstance->UpdateDisplay (DataItem::TWA , TWA);
        break;
    }

    case N2kWind_True_water:
        windRefStr="True Water";
        break;

    default:
        windRefStr="Error";
        break;
    }

    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
            "Wind Data\n\n \
    Seq\t\t%d\n \
    Wind Speed\t\t%.01fkts\n \
    Wind Direction\t%.01f\n \
    Wind Reference\t%s",
            sequence,msToKnots(windSpeed), RadToDeg(windDirection), windRefStr);
    }
}

//-------------------------------------
// Environmental 130310
//-------------------------------------
void Parser::Handle130310(const tN2kMsg &p_rN2kMsg)
{
    unsigned char SID;
    double WaterTemperature , ambientTemperature , pressure;

    if (ParseN2kPGN130310 (p_rN2kMsg, SID , WaterTemperature , ambientTemperature , pressure ))
    {
        s_pDisplayInstance->UpdateDisplay (DataItem::TEMP , KelvinToC(WaterTemperature));
    }
    if (m_DialogActive)
    {
        std::sprintf (m_buffer ,
                     "Environmental Data\n\n \
    Seq\t\t%d\n \
    Water Temperature\t%.01f°C\n \
    Air Temperature\t%.01f\n \
    Wind Reference\t%0.1f",
             SID, KelvinToC (WaterTemperature), KelvinToC (ambientTemperature), pressure);

    }
}

//--------------------------------------------
// Temperature 130312
//--------------------------------------------
void Parser::Handle130312(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    int SID=p_rN2kMsg.GetByte(offset);
    int TempInstance=p_rN2kMsg.GetByte(offset);
    tN2kTempSource tempSource =(tN2kTempSource)(p_rN2kMsg.GetByte(offset));
    double ActualTemperature=p_rN2kMsg.Get2ByteUDouble(0.01,offset);
    double SetTemperature=p_rN2kMsg.Get2ByteUDouble(0.01,offset);

    const char * tempSourceStr = N2kEnumTypeToStr(static_cast<tN2kTempSource>(tempSource));


    std::sprintf (m_buffer , "Temperature\n\n \
    Seq\t\t%d\n \
    Temp Inst\t%d\n \
    Temp Source\t%s\n \
    Actual Temp\t%.1f\n \
    Set Temp\t%.1f",
        SID , TempInstance , tempSourceStr , ActualTemperature , SetTemperature);
}


//--------------------------------------------
// Environmental 130314
//--------------------------------------------
void Parser::Handle130314(const tN2kMsg & p_rN2kMsg)
{
    int offset = 0;
    int8_t sequence = p_rN2kMsg.GetByte(offset);
    int8_t baroInst = p_rN2kMsg.GetByte(offset);
    int8_t baroSrc = p_rN2kMsg.GetByte(offset);

    double actualbaro = p_rN2kMsg.Get4ByteUDouble(1e-3,offset);
    const char * pressSource = N2kEnumTypeToStr(static_cast<tN2kPressureSource>(baroSrc));

    s_pDisplayInstance->UpdateDisplay(DataItem::BARO , actualbaro);

    if (m_DialogActive)
    {
        std::sprintf (m_buffer , "Environmental Parameters\n\n \
    Seq\t\t%d\n \
    Baro Inst\t\t%d\n \
    Pressure Source\t%s\n \
    Actual Baro\t\t%.1fmb",
            sequence,baroInst, pressSource, actualbaro);
    }
}

//-------------------------------------
//
//-------------------------------------
void Parser::Handle130316(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    unsigned char PressureInstance;
    tN2kPressureSource source;
    double Baro;

    if (ParseN2kPGN130314(N2kMsg,SID,PressureInstance, source, Baro) )
    {
        s_pDisplayInstance->UpdateDisplay(DataItem::BARO , Baro);
    }

}


//--------------------------------------------
// Default Handler
//--------------------------------------------
void Parser::HandleDefault(const tN2kMsg & p_rN2kMsg)
{
    std::ostringstream buf;
    // loop the data
    char buffer [12];
    std::string data;
    int dataLen = p_rN2kMsg.DataLen;
    for (int i = 0 ; i < dataLen ; i++)
    {
        sprintf(buffer , "%02X%c" , p_rN2kMsg.Data[i] , i%8 == 7 ? '\n' : ' ' );
        data+= buffer;
    }

    std::sprintf (m_buffer , "Default Handler\n\n \
    Data\n %s",
        data.data());
}




