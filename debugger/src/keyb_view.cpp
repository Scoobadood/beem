#include "keyb_view.h"
#include "ui_keyb_view.h"

KeybView::KeybView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KeybView)
{
    ui->setupUi(this);
}

KeybView::~KeybView()
{
    delete ui;
}
