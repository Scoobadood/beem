#ifndef DEEBWINDOW_H
#define DEEBWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class DeebWindow; }
QT_END_NAMESPACE

class DeebWindow : public QMainWindow
{
    Q_OBJECT

public:
    DeebWindow(QWidget *parent = nullptr);
    ~DeebWindow();

private:
    Ui::DeebWindow *ui;
};
#endif // DEEBWINDOW_H
