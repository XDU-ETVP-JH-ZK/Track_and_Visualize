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
#include <QRect>
#include <QFont>
#include <QCameraDevice>
#include <QMediaDevices>
//#include <QCameraInfo>

#include <cmath>

#include "pangolin_lib.h"
#include "image_change.h"
//图片缩放
#include "imagewidget.h"
//点击获取像素坐标及图片缩放
#include "imageitem.h"
#include "imagescene.h"
//相机标定
#include "Launch.h"
//追踪
#include "reconstruction.h"
//线程
#include "model_thread.h"
#include "calib_thread.h"
#include "pangolinThread.h"
#include "cameraThread.h"
//子界面
#include "winfocus.h"
#include "slamwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    Images_Change ic;
    void recvShowPicSignal(QImage image, QGraphicsView *view);      //缩放展示图片，可拖拽
    void showpic(QImage pic, QGraphicsView *view);      //缩放展示图片，获取点击处像素坐标，不可拖拽
    void LabelDisplayMat(QLabel *label, cv::Mat &mat);      //label上展示cv::Mat格式S图片
//    Eigen::Matrix3d GetMatrix(QString file);
//    cv::Mat GetMat(QString file, int x, int y);
    double x2y(cv::Point2d x, cv::Point2d y);
    bool online(std::vector<cv::Point2d> w);
    void track(Eigen::Matrix3d H, Eigen::Matrix3d K, cv::Mat distCoeffs, rcs::trackerType ttype,
               rcs::featureType ftype, rcs::solveMethod smethod);
    void closeEvent(QCloseEvent *e);        //关闭事件
    ~MainWindow();

private slots:
    void on_calib_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

//    void on_s2run_clicked();

    void on_loadcamera_clicked();

//    void on_back_clicked();

    void on_track_clicked();

    void on_in_row_editingFinished();

    void on_in_col_returnPressed();

    void on_in_d_editingFinished();

    void on_chosevideo_clicked();

    void on_s1loadpic_clicked();

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

    void showid();      //组件现实句柄
    void showdata(QString);     //日志输出
    void showlog();     //输出错误日志
    void showimg1();        //视频图像显示
    void showimg2();        //匹配图像显示
    void SaveLogFile();     //保存日志文件
    void showend(QString);      //结果文件重命名
    void pbhide();      //按钮不可按
    void load_model_succeed();      //模型加载完成
    void showresult();      //展示标定结果

    void on_camera_clicked();

    void on_cameraid_activated(int index);

    void on_pushButton_clicked();

    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;

    WinFocus *win2;
    SlamWindow *win3;

    ImageWidget *m_Image;
//    QList<QVector3D> normal;
//    QList<QVector3D> vertex;
    QTimer* timer;

    QStringList GetImgList(QString path);        //获取指定路径下的图片列表
    void add(QLineEdit *in1, QLineEdit *in2, int num);      //特征点信息添加到特征点列表中
    void TypeCheck(QLineEdit *le, QRegularExpression rx, QString readme);       //检查输入格式
    void hcalcula();        //解算H

    void init();
    void camerainit();
    void tableinit();
    void fileinit();

signals:
    void change();      //改变光标信号

};
#endif // MAINWINDOW_H
