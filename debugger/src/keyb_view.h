#ifndef KEYB_VIEW_H
#define KEYB_VIEW_H

#include <QWidget>

namespace Ui {
class KeybView;
}

class KeybView : public QWidget
{
    Q_OBJECT

public:
    explicit KeybView(QWidget *parent = nullptr);
    ~KeybView();

private:
    Ui::KeybView *ui;
};

#endif // KEYB_VIEW_H
