#ifndef MEMORY_VIEW_H
#define MEMORY_VIEW_H

#include <QWidget>
#include <cstdint>

namespace Ui {
class MemoryView;
}

class MemoryView : public QWidget {
 Q_OBJECT

 public:
  explicit MemoryView(QWidget *parent = nullptr);
  ~MemoryView();

  void set_memory(const std::shared_ptr<std::vector<uint8_t>> &data);
  void resizeEvent(QResizeEvent *event) override;

 private:
  void layout_content();

  Ui::MemoryView *ui;
  uint16_t first_address_;
  std::shared_ptr<std::vector<uint8_t>> data_;
  QSize addr_size_;
  QSize col_size_;
  uint32_t num_rows_;
  uint32_t num_cols_;
};

#endif // MEMORY_VIEW_H
