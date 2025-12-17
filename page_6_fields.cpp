#include "page_6_fields.h"
#include "ui_page_6_fields.h"

Page_6_Fields::Page_6_Fields(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Page_6_Fields)
{
    ui->setupUi(this);
}

Page_6_Fields::~Page_6_Fields()
{
    delete ui;
}
