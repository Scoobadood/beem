#include "control_view.h"
#include "ui_control_view.h"

ControlView::ControlView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlView)
{
    ui->setupUi(this);
}

ControlView::~ControlView()
{
    delete ui;
}
