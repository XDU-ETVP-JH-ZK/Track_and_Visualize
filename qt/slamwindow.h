#ifndef SLAMWINDOW_H
#define SLAMWINDOW_H

#include <QMainWindow>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QDir>
#include <QDebug>
#include <QFileDialog>


namespace Ui {
class SlamWindow;
}

class SlamWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SlamWindow(QWidget *parent = nullptr);
    ~SlamWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_cameras_activated(int index);

private:
    Ui::SlamWindow *ui;
};

#endif // SLAMWINDOW_H
