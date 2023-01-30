#include "crt_view.h"
#include "ui_crt_view.h"
#include "spdlog/spdlog.h"

#include <QImage>

CrtView::CrtView(QWidget *parent)
        : QWidget(parent) //
        , ui(new Ui::CrtView) //
        , pixel_x_{0} //
        , pixel_y_{0} //
{
  ui->setupUi(this);
  crt_image_ = new QImage(640, 512, QImage::Format_BGR888);
//  ui->lbl_crt->setPixmap(QPixmap::fromImage(*crt_image_));
}

CrtView::~CrtView() {
  delete ui;
}

void CrtView::set_next_pixel(uint8_t r, uint8_t g, uint8_t b) {
  crt_image_->setPixelColor(pixel_x_, pixel_y_, QColor{r, g, b});
  pixel_x_ = std::min(pixel_x_ + 1, 639u);
}

void CrtView::hblank() {
  spdlog::info("CRT   : hblank at pixel {}, {}", pixel_x_, pixel_y_);
  pixel_x_ = 0;
  pixel_y_ = std::min(pixel_y_ + 1, 511u);
}


void CrtView::vblank() {
  spdlog::info("CRT   : vblank at pixel {}, {}", pixel_x_, pixel_y_);
  pixel_x_ = 0;
  pixel_y_ = 0;
//  ui->lbl_crt->setPixmap(QPixmap::fromImage(*crt_image_));

}
