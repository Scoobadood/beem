#include "memory_view.h"
#include "ui_memory_view.h"

#include <QStringBuilder>
#include <QResizeEvent>
#include <QWheelEvent>

MemoryView::MemoryView(QWidget *parent)//
        : QWidget(parent), ui(new Ui::MemoryView)//
        , first_address_{0} //
        , first_row_{0} //
        , num_rows_{0} //
        , num_cols_{0} //
{
  ui->setupUi(this);

  ui->te_memory->setContextMenuPolicy(Qt::NoContextMenu);
  ui->te_memory->setReadOnly(true);
  ui->te_memory->setUndoRedoEnabled(false);
  ui->te_memory->setFont(QFont("Courier", 12));

  auto fm = ui->te_memory->fontMetrics();
  addr_size_ = fm.size(Qt::TextSingleLine, "0000:");
  col_size_ = fm.size(Qt::TextSingleLine, " xx");

  connect(ui->sb_memory, &QScrollBar::valueChanged, this, &MemoryView::scroll_to);
  ui->sb_memory->setFocusPolicy(Qt::StrongFocus);
}

MemoryView::~MemoryView() {
  delete ui;
}

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
  auto pt_size = ui->te_memory->size();
  num_cols_ = std::max(1, ((pt_size.width() - addr_size_.width()) / col_size_.width()));
  num_rows_ = std::max(1, (pt_size.height() / col_size_.height()));
  return num_cols_ * num_rows_;
}

/**
 * Track mouse wheels over the text area and treat as scroll requests
 *
 */
void MemoryView::wheelEvent(QWheelEvent * event){
  QPoint numDegrees = event->angleDelta() / 8;
  if (!numDegrees.isNull()) {
    if( numDegrees.y() > 0)
      ui->sb_memory->triggerAction(QAbstractSlider::SliderSingleStepSub);
    else
      ui->sb_memory->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    return;
  }
  event->ignore();
}
/**
 * A resize ocurred so we request data to fill the window.
 */
void MemoryView::resizeEvent(QResizeEvent *event) {
  auto bytes_needed = compute_data_required();
  ui->sb_memory->setMinimum(0);

  int32_t max_row = (0x10000 - num_cols_ * num_rows_) / num_cols_;
  ui->sb_memory->setMaximum(max_row);

  ui->sb_memory->setPageStep(num_rows_);
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
void MemoryView::layout_content() {
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
  ui->te_memory->setHtml(content);
  update();
}