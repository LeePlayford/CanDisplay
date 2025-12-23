#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QGraphicsScene>
#include "qcgaugewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QcNeedleItem *mCompassNeedle;

    Ui::MainWindow *GetUi() {return ui;}


private:
    Ui::MainWindow *ui;
    QTimer * m_timer;



    QGraphicsScene * gScene;

    QcGaugeWidget * mCompassGauge;
    QcGaugeWidget * mSpeedGauge;
    QcNeedleItem *mSpeedNeedle;


    void SetUpFonts();
    void SetUpWindGauge();

public slots:
    void buttonpushed ();
    void timerexpired ();
    void tabChanged();
};
#endif // MAINWINDOW_H
