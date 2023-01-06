#include "../include/deeb_window.h"
#include "./ui_deeb_window.h"

DeebWindow::DeebWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DeebWindow)
{
    ui->setupUi(this);
}

DeebWindow::~DeebWindow()
{
    delete ui;
}

