#include "disasm_view.h"
#include "ui_disasm_view.h"
#include "disassembler.h"

#include <QWidget>

DisasmView::DisasmView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DisasmView)
{
    ui->setupUi(this);
    ui->txt_disassembly->setCurrentFont(QFont("courier", 12));
}

DisasmView::~DisasmView()
{
    delete ui;
}

QString format_for_display(const Operation& op) {

  QString addr = QString("%1")
      .arg(0,4,16, QChar('0'));
  QString label = QString::fromStdString("");
  QString opc = QString::fromStdString(op.opcode.name);
  QString arg;
  switch( op.opcode.addressing_mode ) {
    case OpCode::Implied:
      arg = "";
      break;
    case OpCode::Immediate:
      arg = QString("#$%1").arg(op.data, 2, 16, QChar('0'));
      break;
    case OpCode::Absolute:
      arg = QString("$%1").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::IndirectIndexedX:
      arg = QString("($%1,X)").arg(op.data, 2, 16, QChar('0'));
      break;
    case OpCode::IndirectIndexedY:
      arg = QString("($%1),Y").arg(op.data, 2, 16, QChar('0'));
      break;
    case OpCode::Indirect:
      arg = QString("($%1)").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::AbsoluteIndexedX:
      arg = QString("$%1,X").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::AbsoluteIndexedY:
      arg = QString("$%1,Y").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::Accumulator:
      arg = QString("A");
      break;
    case OpCode::ZeroPage:
      arg = QString("$%1").arg(op.data, 2, 16);
      break;
    case OpCode::ZeroPageIndexedY:
      arg = QString("$%1,Y").arg(op.data, 2, 16);
      break;
    case OpCode::ZeroPageIndexedX:
      arg = QString("$%1,X").arg(op.data, 2, 16);
      break;
    case OpCode::Relative:
      arg = QString("$%1").arg(0 + 2 + (int8_t) op.data, 4, 16, QChar('0'));
      break;
  }
  return QString("%1%2%3%4")
      .arg( addr, -8, ' ')
  .arg(label, -12, ' ')
  .arg( opc, -4, ' ')
  .arg(arg, -10, ' ');
}

void DisasmView::set_data(std::shared_ptr<std::vector<uint8_t>> memory) {
  memory_ = memory;
  disassemble_from_ = 0;
  uint8_t err = 0;

  auto dis =
  Disassembler::disassemble_all(*memory_, disassemble_from_, err);
  ui->txt_disassembly->clear();
  for( const auto & op : dis) {
    auto formatted_op = format_for_display(op);
    ui->txt_disassembly->append(formatted_op);
  }
}
