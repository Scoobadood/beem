#include "memory_view.h"
#include "spdlog/spdlog.h"

#include <QStringBuilder>
#include <QScrollBar>
#include <QResizeEvent>
#include <QFontDatabase>
#include <QAbstractSlider>
#include <QHBoxLayout>

MemoryView::MemoryView(QWidget *parent)//
    : DataDisplayWidget(parent), first_address_{0} //
    , first_row_{0} //
    , num_rows_{0} //
    , num_cols_{0} //
    , layout_{BYTES} //
{
  auto horizontal_layout = new QHBoxLayout(this);

  te_memory_ = new QTextEdit(this);
  te_memory_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  te_memory_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  te_memory_->setUndoRedoEnabled(false);
  te_memory_->setLineWrapMode(QTextEdit::NoWrap);
  te_memory_->setReadOnly(true);
  te_memory_->setContextMenuPolicy(Qt::NoContextMenu);
  te_memory_->setFont(QFont("Monaco", 12));

  horizontal_layout->addWidget(te_memory_);

  sb_memory_ = new QScrollBar(this);
  sb_memory_->setOrientation(Qt::Vertical);
  sb_memory_->setFocusPolicy(Qt::StrongFocus);

  horizontal_layout->addWidget(sb_memory_);

  auto fm = te_memory_->fontMetrics();
  addr_size_ = fm.size(Qt::TextSingleLine, "0000:");
  col_size_ = fm.size(Qt::TextSingleLine, " xx");
  dbl_col_size_ = fm.size(Qt::TextSingleLine, " xxxx");

  assert(connect(sb_memory_, &QScrollBar::valueChanged, this, &MemoryView::scroll_to));
  setLayout(horizontal_layout);
}

MemoryView::~MemoryView() = default;

/**
 * Replace content of the view with a nice presentation of the given data.
 * @param data
 */
void MemoryView::set_data(const std::vector<uint8_t> &data) {
  data_.clear();
  data_.insert(data_.end(), data.begin(), data.end());
  layout_content();
}

uint32_t MemoryView::compute_data_required() {
  // Compute rough volume of data needed to fill the window
  auto pt_size = te_memory_->size();
  auto w = ((layout_ == BYTES) ? col_size_ : dbl_col_size_).width();
  num_cols_ = std::max(1, ((pt_size.width() - addr_size_.width()) / w));
  num_rows_ = std::max(1, (pt_size.height() / col_size_.height()));
  return num_cols_ * num_rows_ * ((layout_ == BYTES) ? 1 : 2);
}

/**
 * Track mouse wheels over the text area and treat as scroll requests
 *
 */
void MemoryView::wheelEvent(QWheelEvent *event) {
  QPoint numDegrees = event->angleDelta() / 8;
  if (!numDegrees.isNull()) {
    if (numDegrees.y() > 0)
      sb_memory_->triggerAction(QAbstractSlider::SliderSingleStepSub);
    else
      sb_memory_->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    return;
  }
  event->ignore();
}

/**
 * A resize ocurred so we request data to fill the window.
 */
void MemoryView::resizeEvent(QResizeEvent *event) {
  auto bytes_needed = compute_data_required();
  sb_memory_->setMinimum(0);

  int32_t max_row = static_cast<int32_t>(0x10000 - num_cols_ * num_rows_) / num_cols_;
  sb_memory_->setMaximum(max_row);

  sb_memory_->setPageStep(num_rows_);
  emit needs_data(this, first_address_, bytes_needed);
}

/**
 * Scrolling will reveal more data which we need to get.
 * For now naively get the whole lot.
 */
void MemoryView::scroll_to(int32_t row) {
  first_row_ = row;
  auto first_address = first_address_ + (first_row_ * num_cols_);
  emit needs_data(this, first_address, num_rows_ * num_cols_);
}

/**
 * Assumes num_rows and num_cols are correct
 */
void MemoryView::layout_content_bytes() {
  QString content = "";

  auto mem_idx = 0;
  auto addr = first_address_ + (first_row_ * num_cols_);

  while (mem_idx < data_.size()) {
    content
        .append("<font color=\"green\">")
        .append(QString("%1").arg(addr, 4, 16, QChar('0')))
        .append(":</font><font color=\"black\">");

    auto col = 0;
    while (col < num_cols_ && mem_idx < data_.size()) {
      content.append(QString(" %1").arg(data_.at(mem_idx), 2, 16, QChar('0')));
      col++;
      mem_idx++;
    }
    addr += col;
    content.append("</font>");
    if (mem_idx != data_.size()) {
      content.append("<br>");
    }
  }
  te_memory_->setHtml(content);
}

/**
 * Assumes num_rows and num_cols are correct
 */
void MemoryView::layout_content_words() {
  QString content = "";

  auto mem_idx = 0;
  auto addr = first_address_ + (first_row_ * num_cols_);

  while (mem_idx < data_.size()) {
    content
        .append("<font color=\"green\">")
        .append(QString("%1").arg(addr, 4, 16, QChar('0')))
        .append(":</font><font color=\"black\">");

    auto col = 0;
    while (col < num_cols_ && mem_idx < data_.size()) {
      auto val = (data_.at(mem_idx) << 8) | data_.at(mem_idx + 1);
      content.append(QString(" %1").arg(val, 4, 16, QChar('0')));
      mem_idx += 2;
      col ++;
    }
    addr += (col * 2);
    content.append("</font>");
    if (mem_idx != data_.size()) {
      content.append("<br>");
    }
  }
  te_memory_->setHtml(content);
}

void MemoryView::layout_content() {
  if (layout_ == BYTES) {
    layout_content_bytes();
  } else {
    layout_content_words();
  }
  update();
}

void MemoryView::set_layout(MemoryView::Layout layout) {
  if (layout_ != layout) {
    layout_ = layout;
    auto bytes_needed = compute_data_required();
    emit needs_data(this, first_address_, bytes_needed);
  }
}