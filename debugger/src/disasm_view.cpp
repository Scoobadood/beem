#include "disasm_view.h"
#include "ui_disasm_view.h"
#include "disassembler.h"

#include <QWidget>
#include <QTextBlock>
#include <QtConcurrent/QtConcurrent>
#include <utility>

DisasmView::DisasmView(QWidget *parent) //
    : QWidget(parent) //
    , ui(new Ui::DisasmView) //
    , disassembler_{} //
    , disassemble_from_{0} //
    , error_{0} //
{
  ui->setupUi(this);
  ui->txt_dis_addr->setText("0x0000");

  connect(ui->txt_dis_addr, &QLineEdit::editingFinished, this, &DisasmView::dis_start_addr_changed);
  ui->txt_disassembly->setFont(QFont("Courier", 12));
}

DisasmView::~DisasmView() {
  delete ui;
}

/**
 * Format an operation for rendering in the view.
 * @param op
 * @return
 */
std::vector<QString> format_for_display(const Operation &op) {
  QString addr = QString("%1:")
      .arg(op.address, 4, 16, QChar('0'));
  QString label = QString::fromStdString("");
  QString opc = QString::fromStdString(op.opcode.name);
  QString arg;
  switch (op.opcode.addressing_mode) {
    case OpCode::Implied:arg = "";
      break;
    case OpCode::Immediate:arg = QString("#$%1").arg(op.data, 2, 16, QChar('0'));
      break;
    case OpCode::Absolute:arg = QString("$%1").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::IndirectIndexedX:arg = QString("($%1,X)").arg(op.data, 2, 16, QChar('0'));
      break;
    case OpCode::IndirectIndexedY:arg = QString("($%1),Y").arg(op.data, 2, 16, QChar('0'));
      break;
    case OpCode::Indirect:arg = QString("($%1)").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::AbsoluteIndexedX:arg = QString("$%1,X").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::AbsoluteIndexedY:arg = QString("$%1,Y").arg(op.data, 4, 16, QChar('0'));
      break;
    case OpCode::Accumulator:arg = QString("A");
      break;
    case OpCode::ZeroPage:arg = QString("$%1").arg(op.data, 2, 16);
      break;
    case OpCode::ZeroPageIndexedY:arg = QString("$%1,Y").arg(op.data, 2, 16);
      break;
    case OpCode::ZeroPageIndexedX:arg = QString("$%1,X").arg(op.data, 2, 16);
      break;
    case OpCode::Relative:arg = QString("$%1").arg(op.address + 2 + (int8_t) op.data, 4, 16, QChar('0'));
      break;
  }

  QString raw = QString("%1").arg(op.opcode.hex, 2, 16, QChar('0'));
  if (op.opcode.bytes > 1) {
    raw += " " + QString("%1")
        .arg(op.data & 0xff, 2, 16, QChar('0'));
  }
  if (op.opcode.bytes > 2) {
    raw += " " + QString("%1")
        .arg(op.data >> 8, 2, 16, QChar('0'));
  }

  return {QString("%1").arg(addr, -8, ' '),
          QString("%1").arg(label, -12, ' '),
          QString("%1").arg(opc, -4, ' '),
          QString("%1").arg(arg, -10, ' '),
          QString("%1").arg(raw, -10, ' ')
  };
}

/**
 * Disab le the text fields while disassembling.
 */
void DisasmView::disable_view() {
  ui->txt_disassembly->setDisabled(true);
  ui->txt_dis_addr->setDisabled(true);
}

/**
 * Disassembly is complete, extract the data and render it.
 */
void DisasmView::enable_view() {
  ui->txt_disassembly->setDisabled(false);
  ui->txt_dis_addr->setDisabled(false);
}

void DisasmView::start_disassembly() {
  disable_view();

  disassembly_future_ = QtConcurrent::run([&]() {
    uint8_t err;
    auto pc = disassemble_from_;
    auto dis = disassembler_.disassemble_all(*memory_, pc, err);
    QMetaObject::invokeMethod(this, &DisasmView::disassembly_complete, Qt::QueuedConnection);
    return dis;
  });
}

void DisasmView::disassembly_complete() {
  auto dis = disassembly_future_.result();
  ui->txt_disassembly->clear();
  pc_to_row_.clear();
  row_to_pc_.clear();

  uint16_t row = 0;
  for (const auto &op: dis) {
    auto formatted_op = format_for_display(op);

    ui->txt_disassembly->moveCursor(QTextCursor::End);
    ui->txt_disassembly->setTextColor(QColorConstants::Black);
    ui->txt_disassembly->insertPlainText(formatted_op[0]);

    ui->txt_disassembly->moveCursor(QTextCursor::End);
    ui->txt_disassembly->setTextColor(QColorConstants::Blue);
    ui->txt_disassembly->insertPlainText(formatted_op[1]);

    ui->txt_disassembly->moveCursor(QTextCursor::End);
    ui->txt_disassembly->setTextColor(QColorConstants::DarkMagenta);
    ui->txt_disassembly->setFontWeight(QFont::Bold);
    ui->txt_disassembly->insertPlainText(formatted_op[2]);

    ui->txt_disassembly->moveCursor(QTextCursor::End);
    ui->txt_disassembly->setTextColor(QColorConstants::DarkGreen);
    ui->txt_disassembly->setFontWeight(QFont::Normal);
    ui->txt_disassembly->insertPlainText(formatted_op[3]);

    ui->txt_disassembly->moveCursor(QTextCursor::End);
    ui->txt_disassembly->setTextColor(QColorConstants::Black);
    ui->txt_disassembly->insertPlainText(formatted_op[4]);
    ui->txt_disassembly->insertPlainText("\n");

    pc_to_row_.emplace(op.address, row);
    row_to_pc_.emplace(row, op.address);
    ++row;
  }
  set_pc(disassemble_from_);
}

/**
 * If the address changed,parse the text into either a hex or decimal offset.
 */
void DisasmView::dis_start_addr_changed() {
  uint16_t new_addr = 0;
  bool ok;

  auto new_start_addr_text = ui->txt_dis_addr->text();
  if (new_start_addr_text.startsWith("0x")) {
    new_start_addr_text = new_start_addr_text.mid(2, new_start_addr_text.size() - 2);
    new_addr = new_start_addr_text.toInt(&ok, 16);
  } else if (
      new_start_addr_text.startsWith("x") ||
          new_start_addr_text.startsWith("$") ||
          new_start_addr_text.startsWith("&")) {
    new_start_addr_text = new_start_addr_text.mid(1, new_start_addr_text.size() - 1);
    new_addr = new_start_addr_text.toInt(&ok, 16);
  } else {
    new_addr = new_start_addr_text.toInt(&ok, 10);
  }
  if (!ok) new_addr = 0;

  new_addr &= 0xffff;
  if (new_addr != disassemble_from_) {
    disassemble_from_ = new_addr;
    start_disassembly();
  }
}

/**
 * Update the data.
 * @param memory
 */
void DisasmView::set_data(std::shared_ptr<std::vector<uint8_t>> memory) {
  memory_ = std::move(memory);
  disassemble_from_ = 0;
  start_disassembly();
}

void DisasmView::set_pc(uint16_t pc) {
  auto show_row = pc_to_row_.at(disassemble_from_);
  QTextCursor cursor(ui->txt_disassembly->document()->  findBlockByLineNumber(show_row));
  ui->txt_disassembly->setTextCursor(cursor);
  enable_view();
}