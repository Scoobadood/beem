#ifndef CONTROL_VIEW_H
#define CONTROL_VIEW_H

#include <QWidget>

namespace Ui {
class ControlView;
}

class ControlView : public QWidget
{
    Q_OBJECT

public:
    explicit ControlView(QWidget *parent = nullptr);
    ~ControlView();

private:
    Ui::ControlView *ui;
};

#endif // CONTROL_VIEW_H
