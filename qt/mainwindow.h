#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QProcess>
#include <QLineEdit>
#include <QRegularExpression>
#include <QTabBar>
#include <QStyleFactory>

#include <cmath>

#include "pangolin_lib.h"
#include "image_change.h"
#include "imagewidget.h"
#include "imageitem.h"
#include "imagescene.h"
#include "Launch.h"
#include "reconstruction.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    Images_Change ic;
    void recvShowPicSignal(QImage image, QGraphicsView *view);
    void showpic(QImage pic, QGraphicsView *view);
    void LabelDisplayMat(QLabel *label, cv::Mat &mat);
    Eigen::Matrix3d GetMatrix(QString file);
    double x2y(cv::Point2d x, cv::Point2d y);
    bool online(std::vector<cv::Point2d> w);
    cv::Mat GetMat(QString file, int x, int y);
    void track(Eigen::Matrix3d H, Eigen::Matrix3d K, cv::Mat distCoeffs, rcs::trackerType ttype,
               rcs::featureType ftype, rcs::solveMethod smethod);
    ~MainWindow();

private slots:
    void on_one2two_clicked();

    void on_calib_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_two2three_clicked();

    void on_three2four_clicked();

    void on_s2run_clicked();

    void on_chosepic_clicked();

    void on_back_clicked();

    void on_track_clicked();

    void on_hidetest_clicked();

    void on_calcula_clicked();

    void on_in_row_editingFinished();

    void on_in_col_returnPressed();

    void on_in_x_editingFinished();

    void on_in_y_returnPressed();

    void on_two2one_clicked();

    void on_three2two_clicked();

    void on_in_d_editingFinished();

    void on_chosevideo_clicked();

private:
    Ui::MainWindow *ui;
    ImageWidget *m_Image;
};
#endif // MAINWINDOW_H
