#include "reg_view.h"

#include "ui_reg_view.h"

RegisterView::RegisterView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterView)
{
    ui->setupUi(this);
}

RegisterView::~RegisterView()
{
    delete ui;
}
