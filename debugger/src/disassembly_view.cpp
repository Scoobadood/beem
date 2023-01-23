#include "disassembly_view.h"
#include "ui_disassembly_view.h"
#include "spdlog/spdlog.h"

#include <QResizeEvent>
#include <QTextBlock>

struct DisassemblyView::FormattedOperation {
  QString address;
  QString label;
  QString operation;
  QString operand;
  QString bytes;
};

DisassemblyView::DisassemblyView(QWidget *parent) //
        : QWidget(parent) //
        , ui(new Ui::DisassemblyView) //
        , disassembler_{} //
        , current_pc_{0} //
        , row_height_{0} //
        , displayed_rows_{0} //
        , first_displayed_byte_offset_{0} //
{
  ui->setupUi(this);
  ui->te_disassembly->viewport()->installEventFilter(this);
  ui->te_disassembly->setContextMenuPolicy(Qt::NoContextMenu);
  ui->te_disassembly->setReadOnly(true);
  ui->te_disassembly->setUndoRedoEnabled(false);
  ui->te_disassembly->setFont(QFont("Monaco", 12));

  connect(ui->sb_disassembly, &QScrollBar::valueChanged, this, &DisassemblyView::scroll_to);
  ui->sb_disassembly->setFocusPolicy(Qt::StrongFocus);

  auto fm = ui->te_disassembly->fontMetrics();
  row_height_ = fm.size(Qt::TextSingleLine, "0000").height();
}

DisassemblyView::~DisassemblyView() {
  delete ui;
}

void DisassemblyView::update_breakpoints(const std::set<uint16_t> &breakpoints) {
  breakpoint_addresses_.clear();
  breakpoint_addresses_.insert(breakpoints.begin(), breakpoints.end());
  layout_disassembly();
}


/**
 * Override the wheel events in the TextView and redirect to the scroll bar instead
 */
bool DisassemblyView::eventFilter(QObject *obj, QEvent *event) {
  //! Ignore ALL wheel events
  if (event->type() == QEvent::Wheel) {
    QPoint numDegrees = ((QWheelEvent *) event)->angleDelta() / 8;
    if (!numDegrees.isNull()) {
      if (numDegrees.y() > 0)
        ui->sb_disassembly->triggerAction(QAbstractSlider::SliderSingleStepSub);
      else
        ui->sb_disassembly->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    }
    return true;
  }
  if (event->type() == QEvent::MouseButtonPress) {
    mousePressEvent((QMouseEvent *) event);
    return true;
  }
  return false;
}

/**
 * Capture some key attributes about the view and then
 * populate it.
 * @param event
 */
void DisassemblyView::resizeEvent(QResizeEvent *event) {
  auto pt_size = event->size();
  displayed_rows_ = std::max(1, (pt_size.height() / row_height_));

  ui->sb_disassembly->setMinimum(0);

  uint32_t displayed_bytes;
  if (!addr_to_row_.empty()) {
    displayed_bytes = addr_to_row_.rbegin()->first - addr_to_row_.begin()->first + 3;
  } else {
    displayed_bytes = displayed_rows_ * 3;
  }
  ui->sb_disassembly->setMaximum(0x10000 - displayed_bytes);
  ui->sb_disassembly->setPageStep(displayed_bytes);

  auto num_bytes = std::min(displayed_rows_ * 3, 0x10000 - first_displayed_byte_offset_);

  emit needs_data(this, first_displayed_byte_offset_, num_bytes);
}

/**
 * The text view shows multiple bytes of data per row as disassembly. There could be 1 .. 3
 * Scroll to a row should compute a rough estimate of the start of that row in bytes and
 * take it from there
 */
void DisassemblyView::scroll_to(uint16_t address) {
  spdlog::info("scroll_to({:04x})", address);

  first_displayed_byte_offset_ = address;
  auto num_bytes = std::min(displayed_rows_ * 3, 0x10000 - first_displayed_byte_offset_);
  ui->sb_disassembly->blockSignals(true);
  ui->sb_disassembly->setValue(address);
  ui->sb_disassembly->blockSignals(false);
  emit needs_data(this, first_displayed_byte_offset_, num_bytes);
}

bool DisassemblyView::address_on_screen(uint16_t addr, uint32_t *row, float *proportion) {
  auto it = addr_to_row_.find(addr);
  if (it == addr_to_row_.end()) {
    return false;
  }

  auto display_row = it->second;
  if (row) *row = display_row;
  if (proportion) *proportion = (float) display_row / (float) displayed_rows_;
  return true;
}

/**
 * Set the PC which will force us to scroll to it and highlight the row.
 * Possible cases:
 * 1. The PC address is disassembled and on screen in the centre.
 *    -> Highlight it and finish
 * 1a. The PC Address is disassembled but towards the bottom of the screen
 *    -> Scroll by a handful of rows by
 *       -> Pick the number of rows to scroll by
 *       -> Get the address of the line that many rows down
 *       -> Add 3n bytes and do the fetch with that
 *
 * 2. The PC address is on screen but the disassembly is incorrect.
 *    -> Find a new disassembly that makes the PC a legit address
 *    -> Display new layout
 * 3. The PC is not on screen
 *    -> Guesstimate a range of addresses that contain the PC and request more data
 *    -> Disassemble when set
 *
 */
void DisassemblyView::set_pc(uint16_t pc) {
  if (displayed_rows_ == 0) return;

  uint32_t row;
  float prop;
  if (!address_on_screen(pc, &row, &prop)) {
    // Desired PC is not on screen, scroll to display it.
    // This will clear any old PC automatically.
    current_pc_ = pc;
    if (displayed_rows_ / 2 > current_pc_) {
      scroll_to(0);
    } else {
      scroll_to(current_pc_ - (displayed_rows_ / 2));
    }
    return;
  }

  // If the PC if in the bottom couple of lines of the screen,
  // Just scroll to it
  if (prop > 0.8) {
    current_pc_ = pc;
    auto next_addr = row_to_addr_.at(1);
    scroll_to(next_addr);
  }

  // PC is on screen and high enough up that we aren;t going to scroll
  // We may need to unhighlight the old PC before highlighting the new one.
  uint32_t old_pc_row;
  auto old_pc = current_pc_;
  current_pc_ = pc;
  if (address_on_screen(old_pc, &old_pc_row)) {
    auto blk = ui->te_disassembly->document()->findBlockByLineNumber(old_pc_row);
    auto cursor = QTextCursor(blk);
    redraw(cursor, false);
  }
  auto blk = ui->te_disassembly->document()->findBlockByLineNumber(row);
  auto cursor = QTextCursor(blk);
  redraw(cursor, true);
}

/**
 * Format an operation for rendering in the view.
 * This takes care only of building the individual text strings. It doe not apply any styling.
 */
DisassemblyView::FormattedOperation DisassemblyView::format_for_display(const Operation &op) {
  QString addr = QString("%1:")
          .arg(op.address, 4, 16, QChar('0'));

  QString label = QString::fromStdString("");
  auto iter = symbols_.find(op.address);
  if (iter != symbols_.end()) {
    label = iter->second;
  }

  QString opc = QString::fromStdString(op.opcode.name);
  QString arg;
  switch (op.opcode.addressing_mode) {
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
      arg = QString("$%1").arg(op.address + 2 + (int8_t) op.data, 4, 16, QChar('0'));
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
 * Highlight the row containing the PC if present
 */
void DisassemblyView::redraw(QTextCursor cursor, bool is_pc) {
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 1);
  cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
  QTextCharFormat fmt{};
  fmt.setForeground(QBrush{is_pc ? QColorConstants::LightGray : QColorConstants::White});
  cursor.setCharFormat(fmt);
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  update();
}

void DisassemblyView::clear_brkpt_formatting(QTextCursor cursor) {
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
  cursor.insertText(" ");
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  update();
}

void DisassemblyView::set_brkpt_formatting(QTextCursor cursor) {
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
  QTextCharFormat fmt{};
  fmt.setForeground(QBrush{QColorConstants::Red});
  cursor.setCharFormat(fmt);
  cursor.insertText("*");
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  update();
}

void DisassemblyView::disassemble_data(const std::vector<uint8_t> &data) {
  disassembler_.set_base_address(first_displayed_byte_offset_);

  uint8_t err = 0;
  uint16_t offset = 0;

  addr_to_row_.clear();
  row_to_addr_.clear();
  disassembly_.clear();

  auto rows = 0;
  while (rows < displayed_rows_) {
    auto dis = disassembler_.disassemble_one(data, offset, err);
    if (err != 0) break;
    disassembly_.emplace_back(dis);
    addr_to_row_.emplace(dis.address, rows);
    row_to_addr_.emplace(rows, dis.address);
    rows++;
  }
}


bool DisassemblyView::has_breakpoint(uint16_t addr) const {
  return breakpoint_addresses_.find(addr) != breakpoint_addresses_.end();
}

/**
 *
 */
void DisassemblyView::layout_disassembly() {
  ui->te_disassembly->clear();

  auto address_colour = QColorConstants::DarkGreen;
  auto label_colour = QColorConstants::DarkGreen;
  auto op_colour = QColorConstants::DarkBlue;
  auto oper_colour = QColorConstants::DarkMagenta;
  auto bytes_colour = QColorConstants::LightGray;

  for (const auto &op: disassembly_) {
    auto formatted_op = format_for_display(op);

    ui->te_disassembly->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    if (has_breakpoint(op.address)) {
      ui->te_disassembly->setTextColor(QColorConstants::Red);
      ui->te_disassembly->insertPlainText("*");// For BP Marker
    } else {
      ui->te_disassembly->insertPlainText(" ");// For BP Marker
    }

    if (op.address == current_pc_) {
      ui->te_disassembly->setTextBackgroundColor(QColorConstants::LightGray);
    } else {
      ui->te_disassembly->setTextBackgroundColor(QColorConstants::White);
    }

    ui->te_disassembly->setTextColor(address_colour);
    ui->te_disassembly->insertPlainText(formatted_op.address);

    ui->te_disassembly->setTextColor(label_colour);
    ui->te_disassembly->insertPlainText(formatted_op.label);

    ui->te_disassembly->setTextColor(op_colour);
    ui->te_disassembly->insertPlainText(formatted_op.operation);

    ui->te_disassembly->setTextColor(oper_colour);
    ui->te_disassembly->insertPlainText(formatted_op.operand);

    ui->te_disassembly->setTextColor(bytes_colour);
    ui->te_disassembly->insertPlainText(formatted_op.bytes);

    ui->te_disassembly->setTextBackgroundColor(QColorConstants::White);

    ui->te_disassembly->insertPlainText("\n");
  }
  ui->te_disassembly->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
}

/**
 * Turn the presented data into a disassembly
 * @param data
 */
void DisassemblyView::set_data(const std::vector<uint8_t> &data) {
  disassemble_data(data);
  layout_disassembly();
}

void DisassemblyView::mousePressEvent(QMouseEvent *e) {
  auto cursor = ui->te_disassembly->cursorForPosition(e->pos());
  auto line = cursor.blockNumber();
  auto it = row_to_addr_.find(line);
  if (it == row_to_addr_.end()) return;

  if (has_breakpoint(it->second)) {
    clear_brkpt_formatting(cursor);
    breakpoint_addresses_.erase(it->second);
    emit breakpoint_cleared(it->second);
  } else {
    set_brkpt_formatting(cursor);
    breakpoint_addresses_.emplace(it->second);
    emit breakpoint_set(it->second);
  }
}