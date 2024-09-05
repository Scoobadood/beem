//
// Created by Dave Durbin on 9/1/2023.
//

#include "disx_view.h"
#include "disassembler.h"
#include "moc_disassembly_view.cpp"

#include <QWidget>
#include <QTextBlock>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>
#include <utility>

DisView::DisView(QWidget *parent) //
    : QPlainTextEdit(parent) //
    , disassembler_{} //
    , disassemble_from_{0} //
    , current_pc_{0} //
    , displayed_rows_{0} //
    , first_displayed_row_{0} //
    , first_displayed_byte_offset_{0} //
    , first_address_{0} //
{
  setContextMenuPolicy(Qt::NoContextMenu);
  setReadOnly(true);
  setUndoRedoEnabled(false);
  setFont(QFont("Courier", 12));
}

DisView::~DisView() = default;

/**
 * Format an operation for rendering in the view.
 * @param op
 * @return
 */
std::vector<QString> DisView::format_for_display(const Operation &op) {
  QString addr = QString("%1:")
      .arg(op.address, 4, 16, QChar('0'));

  QString label = QString::fromStdString("");
  auto iter = symbols_.find( op.address);
  if(iter!= symbols_.end()) {
    label = iter->second;
  }

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
 * Disassemble sufficient data to populate one screen.
 */
void DisView::update_disassembly() {
  setUpdatesEnabled(false);

  clear();

  auto row = 0;
  for (const auto &op: disassembly_) {
    row_to_addr_.emplace(row, op.address);
    addr_to_row_.emplace(op.address, row);
    row++;

    // Format and update the UI
    auto formatted_op = format_for_display(op);

    auto addr_colour = palette().color(QPalette::WindowText).name();
    auto op_colour = palette().color(QPalette::Text).name();
    auto oper_colour = palette().color(QPalette::BrightText).name();

    QString str = "<pre>";
    str.append("<font color=\"" + addr_colour + "\">")
        .append(formatted_op[0])
        .append("</font><font color=\"" + addr_colour + "\">")
        .append(formatted_op[1])
        .append("</font><font color=\"" + op_colour + "\">")
        .append(formatted_op[2])
        .append("</font><font color=\"" + oper_colour + "\">")
        .append(formatted_op[3])
        .append("</font><font color=\"" + addr_colour + "\">")
        .append(formatted_op[4])
        .append("</font></pre>");

    appendHtml(str);
  }
  setUpdatesEnabled(true);
  update();
}

void DisView::set_symbols(const std::map<uint16_t, std::string> &symbols) {
  symbols_.clear();
  for( const auto & symbol : symbols) {
    symbols_.emplace(symbol.first, QString::fromStdString(symbol.second));
  }
  auto last_cursor = textCursor();
  auto line_number = last_cursor.blockNumber();
  update_disassembly();
  QTextCursor cursor(document()->findBlockByLineNumber(line_number)); // ln-1 because line number starts from 0
  setTextCursor(cursor);

}

void DisView::set_current_address(uint16_t pc) {
  // Deselect
  auto cursor = textCursor();
  auto f = textCursor().blockFormat();
  f.setBackground(QColorConstants::White);
  cursor.setBlockFormat(f);

  // Make sure disassembly is correct
  auto iter = addr_to_row_.find(pc);
  if (iter == addr_to_row_.end()) {
    disassemble_from_ = pc;
    update_disassembly();
    iter = addr_to_row_.find(pc);
  }

  auto display_row = iter->second;
  auto first_line = firstVisibleBlock().blockNumber();
  QPoint bottom_right(viewport()->width() - 1, viewport()->height() - 1);
  auto last_line = cursorForPosition(bottom_right).blockNumber();
  auto lines_displayed = (last_line - first_line) + 1;
  auto quarter_screen = lines_displayed / 4;

  cursor = QTextCursor(document()->findBlockByLineNumber(display_row));

  // If the display row is not visible, scroll it to 1/4 way down screen.
  if (display_row < first_line) {
    auto scroll_delta = display_row - quarter_screen - first_line;
    verticalScrollBar()->setValue(verticalScrollBar()->value() + scroll_delta);
  } else if (display_row >= last_line) {
    auto scroll_delta = display_row - last_line + quarter_screen;
    verticalScrollBar()->setValue(verticalScrollBar()->value() + scroll_delta);
  } else if (display_row + quarter_screen > last_line) {
    verticalScrollBar()->setValue(verticalScrollBar()->value() + 1);
  }
  setTextCursor(cursor);

  f = cursor.blockFormat();
  f.setBackground(QColor(151, 212, 240));
  cursor.setBlockFormat(f);
}

/**
 * Update the data.
 * @param memory
 */
void DisView::set_data(const std::vector<uint8_t> & data, uint16_t base_address) {
  data_.clear();
  data_.insert(data_.end(), data.begin(), data.end());

  disassembler_.set_base_address(base_address);

  disassembly_.clear();
  clear();

  disassemble_from_ = 0;

  row_to_addr_.clear();
  addr_to_row_.clear();

  disassembly_ = disassembler_.disassemble_all(data_, disassemble_from_, error_);

  update_disassembly();
}

void DisView::set_bp_formatting(Operation &op, QTextCursor &cursor) {
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  auto formatted_op = format_for_display(op);
  QString str = "<pre>";

  auto addr_colour = palette().color(QPalette::WindowText).name();
  auto op_colour = palette().color(QPalette::Text).name();
  auto oper_colour = palette().color(QPalette::BrightText).name();
  str.append("(<font color=\"" + addr_colour + R"(" font-weight="bold">)")
      .append(formatted_op[0])
      .append("</font><font color=\"" + op_colour + "\">")
      .append(formatted_op[1])
      .append("</font><font color=\"" + op_colour + "\">")
      .append(formatted_op[2])
      .append("</font><font color=\"" + oper_colour + "\">")
      .append(formatted_op[3])
      .append("</font><font color=\"" + op_colour + "\">")
      .append(formatted_op[4])
      .append("</font></pre>");

  cursor.insertHtml(str);
  cursor.block().setUserState(1);
}

void DisView::clear_bp_formatting(Operation &op, QTextCursor &cursor) {
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  auto formatted_op = format_for_display(op);

  auto addr_colour = palette().color(QPalette::WindowText).name();
  auto op_colour = palette().color(QPalette::Text).name();
  auto oper_colour = palette().color(QPalette::BrightText).name();

  QString str = "<pre>";
  str.append(R"(<font color="red" font-weight="bold">)")
      .append(formatted_op[0])
      .append("</font><font color=\"" + addr_colour + "\">")
      .append(formatted_op[1])
      .append("</font><font color=\"" + op_colour + "\">")
      .append(formatted_op[2])
      .append("</font><font color=\"" + oper_colour + "\">")
      .append(formatted_op[3])
      .append("</font><font color=\"" + addr_colour + "\">")
      .append(formatted_op[4])
      .append("</font></pre>");
  cursor.insertHtml(str);
  cursor.block().setUserState(0);
}

void DisView::mousePressEvent(QMouseEvent *e) {
  auto cursor = cursorForPosition(e->pos());
  auto line = cursor.blockNumber();
  auto it = row_to_addr_.find(line);
  if (it == row_to_addr_.end()) return;

  auto op = disassembly_[line];
  if (cursor.block().userState() == 1) {
    clear_bp_formatting(op, cursor);
    emit breakpoint_cleared(it->second);
    breakpoint_lines_.erase(line);
  } else {
    emit breakpoint_set(it->second);
    breakpoint_lines_.emplace(line);
    set_bp_formatting(op, cursor);
  }
}