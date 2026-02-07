#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <QList>
#include <QMap>

#include "N2kMsg.h"

#include "ProcessCancelHelper.h"


namespace Ui
{
    class MainWindow;
    class Dialog;
};
class QDialog;
class tNMEA2000_SocketCAN;
//----------------------------
//
//----------------------------

class DeviceItem
{
public:
    DeviceItem () { deviceName = "Unknown";}
    QString deviceName;
    QMap<unsigned long , const tN2kMsg*> PGNList;
};


//----------------------------
//
//----------------------------
class Parser : public QObject
{
    Q_OBJECT
public:
    explicit Parser(Ui::MainWindow * ui , Ui::Dialog * dlg,tNMEA2000_SocketCAN * p_SocketCan);
    ~Parser();
    void AddDeviceName (int node , QString & name);
    void AddPGN (const tN2kMsg & p_rN2kMsg);
    void Update();
    QList<int> GetMissingProductData()
    {
        return m_missingProductData;
    }
    void SetSeconds (unsigned long seconds) { m_secondsSinceMidnight = seconds;}


private:
    Ui::MainWindow * m_pUi;
    Ui::Dialog * m_pDlg;
    bool m_DialogActive;
    QDialog * m_pDisplayDlg;
    tNMEA2000_SocketCAN * m_pSocketCan;
    void UpdateDisplay (const tN2kMsg & p_rN2kMsg);
    void UpdateDialog (const tN2kMsg & p_rN2kMsg);

    QMap<int , DeviceItem> m_deviceList;
    QList<int> m_missingProductData;
    unsigned long m_secondsSinceMidnight;

    int m_currentNode;
    unsigned long m_currentPGN;
    char * m_buffer;
    bool m_loggingData;
    ProcessCancelHelper m_processHelper;

    bool m_dateTimeSet;
    bool IsDateTimeSet() {return m_dateTimeSet;}
    void SetDateTime() { m_dateTimeSet = true;}
    void CleanString(char * str , int length)
    {
        for (int i = 0 ; 9 < length ; i++)
        {
            if (static_cast<uint8_t>(str[i]) == 0xff ||
                static_cast<uint8_t>(str[i]) == 0x0)
            {
                str[i] = 0;break;
            }
        }
    }



    void Handle59904(const tN2kMsg& p_rN2kMsg);
    void Handle60928(const tN2kMsg& p_rN2kMsg);
    void Handle126992(const tN2kMsg& p_rN2kMsg);
    void Handle126996(const tN2kMsg& p_rN2kMsg);
    void Handle126998(const tN2kMsg& p_rN2kMsg);
    void Handle127250(const tN2kMsg& p_rN2kMsg);
    void Handle127258(const tN2kMsg& p_rN2kMsg);
    void Handle127508(const tN2kMsg& p_rN2kMsg);
    void Handle128259(const tN2kMsg& p_rN2kMsg);
    void Handle128267(const tN2kMsg& p_rN2kMsg);
    void Handle128275(const tN2kMsg& p_rN2kMsg);
    void Handle129025(const tN2kMsg& p_rN2kMsg);
    void Handle129026(const tN2kMsg& p_rN2kMsg);
    void Handle129029(const tN2kMsg& p_rN2kMsg);
    void Handle129038(const tN2kMsg& p_rN2kMsg);
    void Handle129039(const tN2kMsg& p_rN2kMsg);
    void Handle129291(const tN2kMsg& p_rN2kMsg);
    void Handle129794(const tN2kMsg& p_rN2kMsg);
    void Handle129809(const tN2kMsg& p_rN2kMsg);
    void Handle129810(const tN2kMsg& p_rN2kMsg);
    void Handle130306(const tN2kMsg& p_rN2kMsg);
    void Handle130310(const tN2kMsg& p_rN2kMsg);
    void Handle130312(const tN2kMsg& p_rN2kMsg);
    void Handle130314(const tN2kMsg& p_rN2kMsg);
    void Handle130316(const tN2kMsg& p_rN2kMsg);
    void HandleDefault(const tN2kMsg& p_rN2kMsg);

private slots:
    void UpdatePGNs();
    void PGNClicked();
    void on_pushButton_shutdown_clicked();
    void on_pushButton_save_clicked();
    void on_pushButton_clear_clicked();
    void on_pushButton_logdata_clicked();

signals:
};

#endif // PARSER_H
