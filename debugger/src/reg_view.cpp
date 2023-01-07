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

void RegisterView::set_cpu(M6502 * cpu) {
  cpu_ = cpu;
  old_pc_ = cpu_->PC();
  old_sp_ = cpu->SP();
  old_flags_ = cpu->flags();
  update_flags();
}

void RegisterView::update_flags() {
  if( ui->lblPC) {
    if (ui->lblPC->text().toInt() == cpu_->PC()) {
      ui->lblPC->setStyleSheet("color:black");
    } else {
      ui->lblPC->setStyleSheet("color:red; font-weight: bold");
      ui->lblPC->setText(QString(cpu_->PC()));
      old_pc_ = cpu_->PC();
    }
  }
  update();
}