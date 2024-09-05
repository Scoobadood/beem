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
  ~MemoryView() override;

  void set_data(const std::vector<uint8_t> &data);
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent * event) override;

signals:
  void needs_data(QWidget * src, uint16_t start_addr, uint32_t num_bytes );

 private:
  void layout_content();
  uint32_t compute_data_required();
  void scroll_to(int32_t address);

  Ui::MemoryView *ui;

  /* Displayed address of first byte in memory */
  uint16_t first_address_;

  /* Row number of the first displayed row 0x0000 - 0xffff */
  uint32_t first_row_;

  /* Data for the current page. Should be just enough to fill */
  std::vector<uint8_t> data_;

  /* Sizes on screen of rendered columns and address */
  QSize addr_size_;
  QSize col_size_;

  /* Dimension of the screen in lines of characters */
  uint32_t num_rows_;
  uint32_t num_cols_;
};

#endif // MEMORY_VIEW_H
