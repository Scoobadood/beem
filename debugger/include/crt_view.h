#ifndef CRT_VIEW_H
#define CRT_VIEW_H

#include <QWidget>

namespace Ui {
class CrtView;
}

class CrtView : public QWidget
{
    Q_OBJECT

public:
    explicit CrtView(QWidget *parent = nullptr);
    ~CrtView() override;

    void set_next_pixel( uint32_t rgb);

private:
    Ui::CrtView *ui;
    uint32_t pixel_x_;
    uint32_t pixel_y_;
    QPixmap * crt_image_;
};

#endif // CRT_VIEW_H
