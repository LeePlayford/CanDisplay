#include "devices.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "qdebug.h"

#include <fstream>
#include <iostream>

QMap<unsigned long , QString> PGN2Str {
    {59392 , QString("ISO Acknowledgement")},
    {59904 , QString("ISO Address Claim")},
    {60928 , QString("ISO Address Claim")},
    {126992 , QString("System Time")},
    {126993 , QString("Heartbeat")},
    {126996 , QString("Product Information")},
    {127245 , QString("Rudder")},
    {127250 , QString("Vessel Heading")},
    {127258 , QString("Magnetic Variation")},
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
    {129539 , QString("GNSS DOP")},
    {129793 , QString("AIS Class A UTC and Date Report")},
    {129797 , QString("AIS Class A Static and Voyage Data")},
    {130306 , QString("Wind Data")},
    {130314 , QString("Actual Pressure")},
    {130577 , QString("Direction Data")},

};

//-----------------------------
//
//-----------------------------
devices::devices(Ui::MainWindow * ui)
{
    m_pUi = ui;
    connect (m_pUi->listWidget_Devices , &QListWidget::itemClicked , this , &devices::UpdatePGNs);
    connect (m_pUi->pushButton_shutdown , &QPushButton::clicked , this , &devices::on_pushButton_shutdown_clicked);
    connect (m_pUi->pushButton_save , &QPushButton::clicked , this , &devices::on_pushButton_save_clicked);
}

//-----------------------------
//
//-----------------------------
void devices::AddDeviceName (int node , QString & name)
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
void devices::AddPGN (int node , unsigned long PGN)
{
    if (!m_deviceList.contains(node))
    {
        m_deviceList[node].PGNList.push_back(PGN);
    }
    else
    {
        if (m_deviceList.contains(node))
        {
            if (!m_deviceList[node].PGNList.contains(PGN))
            {
                m_deviceList[node].PGNList.push_back(PGN);
            }
        }
    }
    if (m_deviceList[node].deviceName == "Unknown")
    {
        if (!m_missingProductData.contains(node))
            m_missingProductData.push_back(node);
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
        m_pUi->listWidget_Devices->update();
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
            //QList<unsigned long> sorted = std::sort
            qSort(item->PGNList.begin(), item->PGNList.end() , [=] (unsigned long & p1 , unsigned long & p2)->bool {return p1 < p2;});
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
            break;
        }
    }
    m_pUi->listWidget_PGNS->update();
}

//--------------------------------------------
//
//--------------------------------------------
void devices::on_pushButton_shutdown_clicked()
{
#if defined (__AARCH64EL__)
    system("sudo shutdown -h now");
#endif

}

//--------------------------------------------
//
//--------------------------------------------
void devices::on_pushButton_save_clicked()
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
            if (PGN2Str.contains(*pgn))
                line = QString("\tPGN : %1 - %2").arg(*pgn).arg(PGN2Str[*pgn]);
            else
                line = QString("\tPGN : %1 - Unknown").arg(*pgn);
            dataFile << line.toStdString() << std::endl;
        }
    }
    dataFile.close();
}



