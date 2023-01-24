#include "crt_view.h"
#include "ui_crt_view.h"

CrtView::CrtView(QWidget *parent)
        : QWidget(parent) //
        , ui(new Ui::CrtView) //
        , pixel_x_{0} //
        , pixel_y_{0} //
{
  ui->setupUi(this);
  crt_image_ = new QPixmap();
}

CrtView::~CrtView() {
  delete ui;
}

void CrtView::set_next_pixel(uint32_t rgb) {

}
