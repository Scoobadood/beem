#include "memory_view.h"
#include "ui_memory_view.h"

#include <QStringBuilder>

MemoryView::MemoryView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MemoryView) {
  ui->setupUi(this);

  ui->te_memory->setContextMenuPolicy(Qt::NoContextMenu);
  ui->te_memory->setReadOnly(true);
  ui->te_memory->setUndoRedoEnabled(false);
  ui->te_memory->setFont(QFont("Courier", 12));

  auto fm = ui->te_memory->fontMetrics();
  addr_size_ = fm.size(Qt::TextSingleLine, "0000:");
  col_size_ = fm.size(Qt::TextSingleLine, " xx");
}

MemoryView::~MemoryView() {
  delete ui;
}

void MemoryView::set_memory(const std::shared_ptr<std::vector<uint8_t>> &data) {
  data_ = data;
  layout_content();
}


void MemoryView::resizeEvent(QResizeEvent *event) {
  auto fm = ui->te_memory->fontMetrics();
  addr_size_ = fm.size(Qt::TextSingleLine, "0000:");
  col_size_ = fm.size(Qt::TextSingleLine, " xx");
  layout_content();
}

void MemoryView::layout_content() {
  // Get dimensions of window.
  auto pt_size = ui->te_memory->size();

  num_cols_ = std::max(1, ((pt_size.width() - addr_size_.width()) / col_size_.width()));
  num_rows_ = std::max(1, (pt_size.height() / col_size_.height()));

  QString content = "";
  auto mem_idx = 0;
  auto addr = first_address_;
  while( mem_idx < data_->size()) {
    content
    .append("<font color=\"green\">")
    .append(QString("%1").arg(addr, 4, 16, QChar('0')))
    .append(":</font><font color=\"black\">");

    auto col = 0;
    while( col < num_cols_ && mem_idx < data_->size()) {
      content.append(QString(" %1").arg( data_->at(mem_idx),2, 16, QChar('0')));
      col++;
      mem_idx++;
    }
    addr += col;
    content.append("</font>");
    if( mem_idx != data_->size()) {
      content.append("<br>");
    }
  }
  ui->te_memory->setHtml(content);
  update();
}