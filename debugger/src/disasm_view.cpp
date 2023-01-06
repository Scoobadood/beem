#include "disasm_view.h"
#include "ui_disasm_view.h"
#include "disassembler.h"

#include <QWidget>

DisasmView::DisasmView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DisasmView)
{
    ui->setupUi(this);
}

DisasmView::~DisasmView()
{
    delete ui;
}

void DisasmView::set_data(std::shared_ptr<std::vector<uint8_t>> memory) {
  memory_ = memory;
  disassemble_from_ = 0;
  uint8_t err = 0;
  Disassembler::disassemble_all(*memory_, disassemble_from_, err);
}
