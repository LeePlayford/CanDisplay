#include "devices.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "qdebug.h"

QMap<unsigned long , QString> PGN2Str {
    {126996 , QString("Product Information")},
    {126993 , QString("Heartbeat")},
    {130577 , QString("Direction Data")},
    {129026 , QString("COG & SOG, Rapid Update")},
    {129029 , QString("GNSS Position Data")},
    {127250 , QString("Vessel Heading")},
    {129038 , QString("AIS Class A Position Report")},
    {129025 , QString("Position, Rapid Update")},
    {127245 , QString("Rudder")},
    {129033 , QString("Time Offset")},
    {126992 , QString("System Time")},
    {128267 , QString("Water Depth")},
    {127258 , QString("Magnetic Variation")},
    {130306 , QString("WindData")},
    {128259 , QString("Speed Water Referenced")},
    {60928 , QString("ISO Address Claim")},
    {130314 , QString("Actual Pressure")},
    {59904 , QString("ISO Address Claim")},
};

//-----------------------------
//
//-----------------------------
devices::devices(Ui::MainWindow * ui)
{
    m_pUi = ui;
    connect (m_pUi->listWidget_Devices , &QListWidget::itemClicked , this , &devices::UpdatePGNs);
}

//-----------------------------
//
//-----------------------------
void devices::AddDeviceName (int node , QString & name)
{
    if (!m_deviceList.contains(node))
    {
        m_deviceList[node].deviceName = name;
        Update();
    }
}
//-----------------------------
//
//-----------------------------
void devices::AddPGN (int node , unsigned long PGN)
{
    if (m_deviceList.contains(node))
    {
        if (!m_deviceList[node].PGNList.contains(PGN))
        {
            m_deviceList[node].PGNList.push_back(PGN);
        }
    }
}


//-----------------------------
//
//-----------------------------
void devices::Update()
{
    m_pUi->listWidget_Devices->clear();
    for (auto item = m_deviceList.begin(); item != m_deviceList.end() ; ++item)
    {
        QString deviceName = QString ("(%1) - %2").arg(item.key()).arg(item->deviceName);
        m_pUi->listWidget_Devices->addItem(deviceName);
    }
}

//-----------------------------
//
//-----------------------------
void devices::UpdatePGNs()
{
    m_pUi->listWidget_PGNS->clear();
    QString curSel = m_pUi->listWidget_Devices->currentItem()->text();
    qDebug() << curSel;
    for (auto item = m_deviceList.begin(); item != m_deviceList.end() ; ++item)
    {
        if (curSel.contains(item->deviceName))
        {
            foreach (auto  pgn , item->PGNList)
            {
                QString pgnName = QString::number(pgn);
                if (PGN2Str.contains(pgn))
                {
                    pgnName += " - ";
                    pgnName += PGN2Str[pgn];
                }
                m_pUi->listWidget_PGNS->addItem(pgnName);
            }
        }
    }


    //if (m_deviceList.count()) <
    //foreach (auto & item : m_deviceList[curSel])
}
