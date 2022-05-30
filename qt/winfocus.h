#ifndef WINFOCUS_H
#define WINFOCUS_H

#include <QMainWindow>

#include <QFileDialog>
#include <QDir>
#include <QFileSystemModel>
#include <QStandardItemModel>

#include "imagewidget.h"

namespace Ui {
class WinFocus;
}

class WinFocus : public QMainWindow
{
    Q_OBJECT

public:
    explicit WinFocus(QWidget *parent = nullptr);
    ~WinFocus();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_4_clicked();

private:
    Ui::WinFocus *ui;
    ImageWidget *m_Image;
    void recvShowPicSignal(QImage image, QGraphicsView *view);      //缩放展示图片，可拖拽
};

#endif // WINFOCUS_H
