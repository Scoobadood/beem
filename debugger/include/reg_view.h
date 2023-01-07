#ifndef REG_VIEW_H
#define REG_VIEW_H

#include <QWidget>

namespace Ui {
class RegisterView;
}

class RegisterView : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterView(QWidget *parent = nullptr);
    ~RegisterView() override;

private:
    Ui::RegisterView *ui;
};

#endif // REG_VIEW_H
