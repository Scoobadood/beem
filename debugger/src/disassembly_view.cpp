//
// Created by Dave Durbin on 9/1/2023.
//

#include "disassembly_view.h"
#include "disassembler.h"

#include <QWidget>
#include <QTextBlock>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>
#include <utility>

DisassemblyView::DisassemblyView(QWidget *parent) //
    : QTextEdit(parent) //
    , disassembler_{} //
    , disassemble_from_{0} //
    , error_{0} //
{
  setFont(QFont("Courier", 12));
  top_row_ = 0;
  last_row_ = 0;
}

DisassemblyView::~DisassemblyView() = default;

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

void DisassemblyView::resizeEvent(QResizeEvent *event) {
  update_disassembly();
}

void DisassemblyView::scrollContentsBy(int dx, int dy){
  QTextEdit::scrollContentsBy(dx, dy);

  auto hvalue = horizontalScrollBar()->value();
  auto vvalue = verticalScrollBar()->value();
  QPoint topLeft = viewport()->rect().topLeft();

  auto c = cursorForPosition(topLeft);
  top_row_ = c.blockNumber();
  update_disassembly();
}

/**
 * Disassemble sufficient data to populate one screen.
 */
void DisassemblyView::update_disassembly() {
  QFontMetrics m(font());
  auto row_height = m.lineSpacing();
  auto num_visible_rows = (height() / row_height) + 1;
  for (auto row = 0; row < num_visible_rows; ++row) {
    auto row_num = top_row_ + row;
    if (row_num > last_row_) break;

    auto iter = row_to_addr_.find(row_num);

    // If the row is already disassembled it should be displayed and we can skip
    if (iter != row_to_addr_.end()) {
      continue;
    }

    // We need to
    //   Workout the address of the operation at this row
    //   Disassemble it
    //   Format it and update row_to_addr_ and addr_to_row_
    //   Update the text field
    // If row is 0, addr is 0 otherwise addr is prev-row-addr + bytes
    uint16_t addr;
    if (row_num == 0) {
      addr = 0;
    } else {
      addr = disassembly_[row_num - 1]->address + disassembly_[row_num - 1]->opcode.bytes;
    }
    uint8_t err;
    auto start_addr = addr;
    auto operation = disassembler_.disassemble_one(*data_, addr, err);
    if (!err) {
      addr_to_row_.emplace(start_addr, row_num);
      row_to_addr_.emplace(row_num, start_addr);
      disassembly_.at(row_num) = std::make_shared<Operation>(operation);
      if (addr >= data_->size()) {
        // Wrapped so that was the last instruction, truncate the vector
        disassembly_.resize(row_num + 1);
        last_row_ = row_num;

        // And truncate the rows of displayed text
        auto tc = textCursor();
        tc.movePosition(QTextCursor::Start);
        tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row_num);
        tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        tc.removeSelectedText();
      }

      // Format and update the UI
      auto formatted_op = format_for_display(operation);

      auto tc = textCursor();
      tc.movePosition(QTextCursor::Start);
      tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row_num); //go down y-times
      setTextCursor(tc);

      setTextColor(QColorConstants::Black);
      insertPlainText(formatted_op[0]);
      setTextColor(QColorConstants::Blue);
      insertPlainText(formatted_op[1]);
      setTextColor(QColorConstants::DarkMagenta);
      setFontWeight(QFont::Bold);
      insertPlainText(formatted_op[2]);
      setTextColor(QColorConstants::DarkGreen);
      setFontWeight(QFont::Normal);
      insertPlainText(formatted_op[3]);
      setTextColor(QColorConstants::Black);
      insertPlainText(formatted_op[4]);

    } else {
      // Error disassembling this address
    }

  }
  update();
}

/**
 * Update the data.
 * @param memory
 */
void DisassemblyView::set_data(std::shared_ptr<std::vector<uint8_t>> memory) {
  data_ = std::move(memory);

  // This is the maximum size needed. in all likelihood less will be used.
  disassembly_.clear();
  disassembly_.resize(data_->size(), nullptr);

  clear();
  setText(QString("\n").repeated(data_->size()));

  disassemble_from_ = 0;
  top_row_ = 0;
  last_row_ = data_->size() - 1;

  row_to_addr_.clear();
  addr_to_row_.clear();

  update_disassembly();
}