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
    QList<int> GetMissingProductData()
    {
        return m_missingProductData;
    }
    void SetSeconds (unsigned long seconds) { m_secondsSinceMidnight = seconds;}


private:
    Ui::MainWindow * m_pUi;
    QMap<int , DeviceItem> m_deviceList;
    QList<int> m_missingProductData;
    unsigned long m_secondsSinceMidnight;

private slots:
    void UpdatePGNs();
    void on_pushButton_shutdown_clicked();
    void on_pushButton_save_clicked();

signals:
};

#endif // DEVICES_H
