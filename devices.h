#ifndef DEVICES_H
#define DEVICES_H

#include <QObject>
#include <QList>
#include <QMap>


namespace Ui
{
    class MainWindow;
};
//----------------------------
//
//----------------------------

class DeviceItem
{
public:
    DeviceItem () { deviceName = "Unknown";}
    QString deviceName;
    QList<unsigned long> PGNList;
};


//----------------------------
//
//----------------------------
class devices : public QObject
{
    Q_OBJECT
public:
    explicit devices(Ui::MainWindow * ui);
    void AddDeviceName (int node , QString & name);
    void AddPGN (int node , unsigned long PGN);
    void Update();


private:
    Ui::MainWindow * m_pUi;
    QMap<int , DeviceItem> m_deviceList;

private slots:
    void UpdatePGNs();

signals:
};

#endif // DEVICES_H
