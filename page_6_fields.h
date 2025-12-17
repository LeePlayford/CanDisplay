#ifndef PAGE_6_FIELDS_H
#define PAGE_6_FIELDS_H

#include <QDialog>

namespace Ui {
class Page_6_Fields;
}

class Page_6_Fields : public QDialog
{
    Q_OBJECT

public:
    explicit Page_6_Fields(QWidget *parent = nullptr);
    ~Page_6_Fields();

private:
    Ui::Page_6_Fields *ui;
};

#endif // PAGE_6_FIELDS_H
