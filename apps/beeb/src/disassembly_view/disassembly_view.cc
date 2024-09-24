#include "disassembly_view.h"
#include "Disassembler/operation_formatter.h"
#include <spdlog/spdlog.h>

#include <QScrollBar>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextEdit>
#include <QThread>
#include <QTextDocumentFragment>
#include <QHBoxLayout>

struct DisassemblyView::FormattedOperation {
  QString address;
  QString label;
  QString operation;
  QString operand;
  QString bytes;
};

struct ColourScheme {
  QColor address_colour = QColorConstants::DarkGreen;
  QColor label_colour = QColorConstants::DarkGreen;
  QColor op_colour = QColorConstants::DarkBlue;
  QColor oper_colour = QColorConstants::DarkMagenta;
  QColor bytes_colour = QColorConstants::LightGray;
  QColor bp_marker_colour = QColorConstants::Red;
  QColor bg_colour = QColorConstants::White;
  QColor pc_colour = QColor{186, 231, 255};
};

const int32_t LS_LABEL = 0x20000;
const int32_t LS_OPCOD = 0x10000;
const int32_t LS_BRKPT = 0x40000;

DisassemblyView::DisassemblyView(BreakpointManager *breakpoint_manager, QWidget *parent) //
    : DataDisplayWidget(parent) //
    , disassembler_{} //
    , current_pc_{0} //
    , row_height_{0} //
    , displayed_rows_{0} //
    , first_displayed_byte_offset_{0} //
    , breakpoint_manager_{breakpoint_manager} //
{
  auto layout = new QHBoxLayout(this);

  te_disassembly_ = new QTextEdit(this);
  te_disassembly_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  te_disassembly_->setLineWrapMode(QTextEdit::NoWrap);
  te_disassembly_->viewport()->installEventFilter(this);
  te_disassembly_->setContextMenuPolicy(Qt::NoContextMenu);
  te_disassembly_->setReadOnly(true);
  te_disassembly_->setUndoRedoEnabled(false);
  te_disassembly_->setFont(QFont("Monaco", 12));
  auto fm = te_disassembly_->fontMetrics();
  row_height_ = fm.size(Qt::TextSingleLine, "0000").height();
  te_disassembly_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  te_disassembly_->setMinimumWidth(48 * fm.maxWidth());

  sb_disassembly_ = new QScrollBar(this);
  sb_disassembly_->setEnabled(true);
  sb_disassembly_->setOrientation(Qt::Orientation::Vertical);
  sb_disassembly_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

  assert(connect(sb_disassembly_, &QScrollBar::valueChanged, this, &DisassemblyView::scroll_to));
  sb_disassembly_->setFocusPolicy(Qt::StrongFocus);

  assert(connect(breakpoint_manager_,
                 &BreakpointManager::breakpoint_cleared,
                 this,
                 &DisassemblyView::breakpoints_changed));
  assert(connect(breakpoint_manager_, &BreakpointManager::breakpoint_set, this, &DisassemblyView::breakpoints_changed));

  layout->addWidget(te_disassembly_);
  layout->addWidget(sb_disassembly_);
  setLayout(layout);

  colour_scheme_ = std::make_unique<ColourScheme>();
}

DisassemblyView::~DisassemblyView() = default;

void DisassemblyView::breakpoints_changed() {
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
        sb_disassembly_->triggerAction(QAbstractSlider::SliderSingleStepSub);
      else
        sb_disassembly_->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    }
    return true;
  }
  if (event->type() == QEvent::MouseButtonPress) {
    mousePressEvent((QMouseEvent *) event);
    return true;
  }
  return false;
}

void DisassemblyView::resize(const QSize &size) {
  spdlog::info("disasm resize() {}x{}   TE is {}x{}",
               size.width(), size.height(),
               te_disassembly_->size().width(),
               te_disassembly_->size().height()
  );
  QWidget::resize(size);
}

/**
 * Capture some key attributes about the view and then
 * populate it.
 * @param event
 */
void DisassemblyView::resizeEvent(QResizeEvent *event) {
  spdlog::info("disasm resizeEvent()  {}x{}   TE is {}x{}",
               event->size().width(), event->size().height(),
               te_disassembly_->size().width(),
               te_disassembly_->size().height()
  );
  auto pt_size = event->size();
  displayed_rows_ = std::max(1, (pt_size.height() / row_height_));

  sb_disassembly_->setMinimum(0);

  uint32_t displayed_bytes;
  if (!addr_to_row_.empty()) {
    displayed_bytes = addr_to_row_.rbegin()->first - addr_to_row_.begin()->first + 3;
  } else {
    displayed_bytes = displayed_rows_ * 3;
  }
  sb_disassembly_->setMaximum(0x10000 - displayed_bytes);
  sb_disassembly_->setPageStep(displayed_bytes);

  auto num_bytes = std::min(displayed_rows_ * 3, 0x10000 - first_displayed_byte_offset_);

  emit needs_data(this, first_displayed_byte_offset_, num_bytes);
}

/**
 * The text view shows multiple bytes of data per row as disassembly. There could be 1 .. 3
 * Scroll to a row should compute a rough estimate of the start of that row in bytes and
 * take it from there
 */
void DisassemblyView::scroll_to(uint16_t address) {
  first_displayed_byte_offset_ = address;
  auto num_bytes = std::min(displayed_rows_ * 3, 0x10000 - first_displayed_byte_offset_);
  sb_disassembly_->blockSignals(true);
  sb_disassembly_->setValue(address);
  sb_disassembly_->blockSignals(false);
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
  if (displayed_rows_ == 0) {
    current_pc_ = pc;
    return;
  }

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

    // Get the second entry in the screen rows which has an address
    // We can't do a look up using row directly because we may have
    // a label at index [1] in te screen and tat won't have a
    // corresponding address.
    auto next_addr = (row_to_addr_.begin()++)->second;
    scroll_to(next_addr);
    return;
  }

  // PC is on screen and high enough up that we aren;t going to scroll
  // We may need to unhighlight the old PC before highlighting the new one.
  uint32_t old_pc_row;
  auto old_pc = current_pc_;
  current_pc_ = pc;
  if (address_on_screen(old_pc, &old_pc_row)) {
    auto blk = te_disassembly_->document()->findBlockByLineNumber(old_pc_row);
    auto cursor = QTextCursor(blk);
    redraw(cursor, false);
  }
  auto blk = te_disassembly_->document()->findBlockByLineNumber(row);
  auto cursor = QTextCursor(blk);
  redraw(cursor, true);

  te_disassembly_->update();
}

/**
 * Format an operation for rendering in the view.
 * This takes care only of building the individual text strings. It doe not apply any styling.
 */
DisassemblyView::FormattedOperation DisassemblyView::format_for_display(const Operation &op) {
  QString addr = QString("%1:").arg(op.address, 4, 16, QChar('0'));
  QString label = QString::fromStdString("");
  QString opc = QString::fromStdString(op.opcode.name);
  QString arg = QString::fromStdString(format_args(op, disassembler_.symbols()));

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
  cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 1);

  QTextCharFormat fmt{};
  fmt.setBackground(QBrush{is_pc ? colour_scheme_->pc_colour : QColorConstants::White});
  cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
  cursor.mergeCharFormat(fmt);
}

void DisassemblyView::clear_brkpt_formatting(QTextCursor cursor) {
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
  cursor.insertText(" ");
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

  auto state = cursor.block().userState();
  state &= ~LS_BRKPT;
  cursor.block().setUserState(state);

  update();
}

void DisassemblyView::set_brkpt_formatting(QTextCursor cursor) {
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
  QTextCharFormat fmt{};
  fmt.setForeground(QBrush{colour_scheme_->bp_marker_colour});
  cursor.insertText("*", fmt);
  cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  auto state = cursor.block().userState();
  state |= LS_BRKPT;
  cursor.block().setUserState(state);
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
    if (err != 0) {
      break;
    }
    disassembly_.emplace_back(dis);
    if( !dis.label.empty())
      ++rows;
    addr_to_row_.emplace(dis.address, rows);
    row_to_addr_.emplace(rows, dis.address);
    ++rows;
  }
}

/**
 * Layout the disassembled opcodes to the screen.
 * This method simply takes the disassembled data and constructs blocks of text
 * which it adds to the UI.
 * We also set flags to determine the type of the block
 * 1: 0x10000 | 16 bit opcode index
 * 2: 0x20000 | 16 bit addr (label only)
 * 4: 0x40000 | 16 bit opcode idx
 * These are used to help handle mouse clicks
 */
void DisassemblyView::layout_disassembly() {
  te_disassembly_->clear();

  auto op_idx = 0;
  for (const auto &op : disassembly_) {
    QString label;
    if (!op.label.empty()) {
      label = QString("%1").arg(QString::fromStdString(op.label), -12, ' ');
      te_disassembly_->setTextColor(colour_scheme_->label_colour);
      te_disassembly_->insertPlainText(label);// For BP Marker
      te_disassembly_->textCursor().block().setUserState(LS_LABEL | op.address);
      te_disassembly_->textCursor().insertBlock();
    }

    auto formatted_op = format_for_display(op);
    auto line_state = LS_OPCOD;
    te_disassembly_->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    if (breakpoint_manager_->is_breakpoint(op.address)) {
      te_disassembly_->setTextColor(colour_scheme_->bp_marker_colour);
      te_disassembly_->insertPlainText("*");
      line_state |= LS_BRKPT;
    } else {
      te_disassembly_->insertPlainText(" ");
    }

    if (op.address == current_pc_) {
      te_disassembly_->setTextBackgroundColor(colour_scheme_->pc_colour);
    } else {
      te_disassembly_->setTextBackgroundColor(colour_scheme_->bg_colour);
    }

    te_disassembly_->setTextColor(colour_scheme_->address_colour);
    te_disassembly_->insertPlainText(formatted_op.address);

    te_disassembly_->setTextColor(colour_scheme_->bytes_colour);
    te_disassembly_->insertPlainText(formatted_op.bytes);

    te_disassembly_->setTextColor(colour_scheme_->op_colour);
    te_disassembly_->insertPlainText(formatted_op.operation);

    te_disassembly_->setTextColor(colour_scheme_->oper_colour);
    te_disassembly_->insertPlainText(formatted_op.operand);

    te_disassembly_->setTextBackgroundColor(colour_scheme_->bg_colour);

    te_disassembly_->textCursor().block().setUserState(line_state | op_idx);

    te_disassembly_->insertPlainText("\n");
    ++op_idx;
  }
  te_disassembly_->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
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
  auto cursor = te_disassembly_->cursorForPosition(e->pos());

  /* Read user state of line */
  auto line_state = cursor.block().userState();
//  if (line_state & LS_LABEL) {
//    // label
//    spdlog::info("The label for address {:04x} {}", (line_state & 0xffff),
//                 symbols_.at(line_state & 0xffff).toStdString());
//  } else if (line_state & LS_OPCOD) {
//    auto op_idx = line_state & 0xffff;
//    auto op = disassembly_.at(op_idx);
//    spdlog::info("The address {:04x} with operation {} {}", op.address, op.opcode.name,
//                 (line_state & LS_BRKPT) ? "with a breakpoint." : "");
//  }

  switch (line_state >> 16) {
    // Label, do nothing
    case 2:
      break;

    case 1: {
      auto op_idx = line_state & 0xffff;
      auto op = disassembly_.at(op_idx);
      auto addr = op.address;
      set_brkpt_formatting(cursor);
      breakpoint_manager_->set_breakpoint(addr);
    }
      break;

    case 5: {
      auto op_idx = line_state & 0xffff;
      auto op = disassembly_.at(op_idx);
      auto addr = op.address;
      clear_brkpt_formatting(cursor);
      breakpoint_manager_->clear_breakpoint(addr);
      emit breakpoint_cleared(addr);
    }
      break;
    default:
      spdlog::info("That's odd. line state shouldn't be  {:07x}", line_state);
      break;

  }
}