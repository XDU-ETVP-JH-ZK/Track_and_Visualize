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
#include <QCloseEvent>
#include <QTimer>
#include <QDateTime>

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
    cv::Mat GetMat(QString file, int x, int y);
    double x2y(cv::Point2d x, cv::Point2d y);
    bool online(std::vector<cv::Point2d> w);
    void track(Eigen::Matrix3d H, Eigen::Matrix3d K, cv::Mat distCoeffs, rcs::trackerType ttype,
               rcs::featureType ftype, rcs::solveMethod smethod);
    void closeEvent(QCloseEvent *e);
    ~MainWindow();

private slots:
    void on_calib_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_s2run_clicked();

    void on_loadcamera_clicked();

//    void on_back_clicked();

    void on_track_clicked();

    void on_calcula_clicked();

    void on_in_row_editingFinished();

    void on_in_col_returnPressed();

    void on_in_d_editingFinished();

    void on_chosevideo_clicked();

    void on_s1loadpic_clicked();

//    void on_point_clicked();
//    void on_point_2_clicked();
//    void on_point_3_clicked();
//    void on_point_4_clicked();
//    void on_point_5_clicked();
//    void on_point_6_clicked();

    void on_point_x_editingFinished();
    void on_point_y_returnPressed();
    void on_point_x_2_editingFinished();
    void on_point_y_2_returnPressed();
    void on_point_x_3_editingFinished();
    void on_point_y_3_returnPressed();
    void on_point_x_4_editingFinished();
    void on_point_y_4_returnPressed();
    void on_point_x_5_editingFinished();
    void on_point_y_5_returnPressed();
    void on_point_x_6_editingFinished();
    void on_point_y_6_returnPressed();

    void on_freeze_clicked();

    void on_LoadModel_clicked();

    void on_ModelUnload_clicked();

    void on_in_d_2_editingFinished();


private:
    Ui::MainWindow *ui;
    ImageWidget *m_Image;
    QList<QVector3D> normal;
    QList<QVector3D> vertex;
    QTimer* timer;

    QStringList GetImgList(QString path);        //获取指定路径下的图片列表
    void add(QLineEdit *in1, QLineEdit *in2, int num);
    void TypeCheck(QLineEdit *le, QRegularExpression rx, QString readme);

};
#endif // MAINWINDOW_H
