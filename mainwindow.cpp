#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "page_6_fields.h"

#include "qdebug.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug () << qVersion();

    //ui->textEdit->setText(qVersion());

    //connect (ui->pushButton , &QPushButton::clicked , this , &MainWindow::buttonpushed);

}

void MainWindow::buttonpushed()
{
    //ui->textEdit->setText("Button Pushed");
    //Page_6_Fields * dlg = new Page_6_Fields(this);
    //dlg->show();

}

MainWindow::~MainWindow()
{
    delete ui;
}
