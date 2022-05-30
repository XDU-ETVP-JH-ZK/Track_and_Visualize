#include "mainwindow.h"
#include "ui_mainwindow.h"

//#include <pcl/point_types.h>
//#include <pcl/io/pcd_io.h>
//#include <pcl/point_cloud.h>
//#include <pcl/visualization/pcl_visualizer.h>

QRegularExpression reg("^[0-9]*[1-9][0-9]*$");     //正整数正则表达式
QRegularExpression reg2("^\\d+$");     //非负整数正则表达式
QRegularExpression reg3("^\\d+(\\.\\d+)?$");     //正数

//QList<QCameraInfo> cameras;
QList<QCameraDevice> cameras;
int cid;
bool choseflag = false;     //step2是否已选择三个点
bool modelload = false;     //是否加载模型
QList<QStringList> pointslist;      //特征点物理坐标和像素坐标列表
QStandardItemModel* model;
int videos = 0;     //视频源,1:摄像头,2:本地视频
Eigen::Matrix3d H;      //单应矩阵
QStringList list;     //step1中切换图片列表
QString imgpath;     //step1选取的路径
QString videopath;     //step2选取的视频路径
std::vector<double> plist;      //每个手动选取角点的像素坐标
int pnum = 0;       //已录入特征点个数
QList<QVector3D> normal;        //模型法线数据
QList<QVector3D> vertex;        //模型坐标数据
QString somename;       //结果文件命名
QString hname;     //解算出的H的文件名
QString tname;     //保存数据格式：四元数或选择矩阵
ImageScene *sc;     //step2画点的scene
std::vector<std::vector<float>> pose;     //保存每帧的r和t
QString ft, sm;     //所选的特征提取和位姿解算算法
rcs::trackerType ttype;     //追踪器类型
rcs::featureType ftype;     //特征提取算法
rcs::solveMethod smethod;       //位姿解算算法
cv::Mat pglFrame;       //视频图像帧
QImage matchimg;        //匹配图像
Eigen::Matrix3d rMat;       //位姿-旋转矩阵
Eigen::Vector3d tVec;       //位姿-平移向量
double cur_t;       //当前t的模
QString errlog;     //错误日志
WId pglwid;     //pangolin窗口句柄
QString lend;       //日志末尾
QString logname, resultname;        //日志文件名，结果文件名

PangolinThread *pt;     //pangolin子线程

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->init();
    this->fileinit();
    this->camerainit();
    this->tableinit();

//    ModelThread *thread1 = new ModelThread();
//    thread1->start();
//    connect(thread1, SIGNAL(succeed()), this, SLOT(load_model_succeed()));

    win2 = new WinFocus(this);
    win3 = new SlamWindow(this);
}


void MainWindow::init()
{
    ui->groupBox->hide();
    ui->groupBox_2->hide();
    ui->modelbox->hide();
    ui->s3hide->hide();     //隐藏标签
    ui->tabWidget->setCurrentIndex(0);     //默认第一页
    /*第三步默认选择pnp、orb和kcf,四元数*/
    ui->pnp->setChecked(true);
    ui->orb->setChecked(true);
    ui->kcf->setChecked(true);
    ui->quat->setChecked(true);
    ui->video->setChecked(true);
}


void MainWindow::fileinit()
{
    QString root = QDir::currentPath() + "/data/";      //新建数据文件夹
    QStringList pathlist;
    pathlist.push_back(root);
    pathlist.push_back(root + "相机标定/");
    pathlist.push_back(root + "初始解算/");
    pathlist.push_back(root + "导航定位/");
    pathlist.push_back(root + "自身位姿估计/");
    pathlist.push_back(root + "病灶检测/");
    for(auto & i : pathlist){
        QDir d(i);
        if(!d.exists())
            d.mkdir(i);
    }
}


void MainWindow::camerainit()
{
//    cameras = QCameraInfo::availableCameras();
    cameras = QMediaDevices::videoInputs();
    for(auto &i : cameras)
        ui->cameraid->addItem(i.description());
}


void MainWindow::on_cameraid_activated(int index)
{
    cid = index;
}


void MainWindow::tableinit()
{
    pointslist.push_back({"0", "0", "0", "0"});pointslist.push_back({"0", "0", "0", "0"});
    pointslist.push_back({"0", "0", "0", "0"});pointslist.push_back({"0", "0", "0", "0"});
    pointslist.push_back({"0", "0", "0", "0"});pointslist.push_back({"0", "0", "0", "0"});
    model = new QStandardItemModel();
    model->setHorizontalHeaderLabels({"物理坐标", "像素坐标"});
    model->setVerticalHeaderLabels({"特征点1","特征点2","特征点3","特征点4","特征点5","特征点6"});
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setModel(model);
    ui->tableView->show();
}


void ModelThread::run()
{
    QFile f("1.stl");
    if(!f.exists()){
        QMessageBox::critical(NULL, "错误", "没找到模型文件", "确定");
        return;
    }
    f.open(QIODevice::ReadOnly);
    while(!f.atEnd()){
        QString line = f.readLine().trimmed();
        QStringList word = line.split(" ", Qt::SkipEmptyParts);
        if(word[0] == "facet")
            normal.append(QVector3D(word[2].toFloat(), word[3].toFloat(), word[4].toFloat()));
        if(word[0] == "vertex")
            vertex.append(QVector3D(word[1].toFloat() +10, word[2].toFloat() - 10, word[3].toFloat() + 500));
    }
    emit succeed();
}


void MainWindow::load_model_succeed()
{
    ui->modelbox->show();
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *e)
{
    int r = QMessageBox::information(this, "退出", "是否退出?", QObject::tr("确认"), QObject::tr("取消"));
    if(r == QObject::tr("确认").toInt())
        this->close();
    else
        e->ignore();
}


void MainWindow::TypeCheck(QLineEdit *le, QRegularExpression rx, QString readme)
{
    if(!le->text().contains(rx)){
        QMessageBox::critical(NULL, "错误", "请输入" + readme, "确定");
        le->clear();
        le->setFocus();
        return;
    }
}

/***********************************************************相机标定*****************************************************************/

std::string PathWithCHN(QString path)       //中文路径转码
{
    QByteArray cdata = path.toLocal8Bit();
    return std::string(cdata);
}

QStringList MainWindow::GetImgList(QString path)        //获取指定路径下的图片列表
{
    QDir dir(path);
    QStringList filter, dl, res;
    filter<<"*.jpg"<<"*.png";
    dl = dir.entryList(filter, QDir::Files | QDir::Readable, QDir::Name);
    for(auto & i : dl)
        res.push_back(path + "/" + i);

    return res;
}

void MainWindow::on_in_d_editingFinished()     //检测输入的边长是不是正整数
{
//    if(!ui->in_d->text().contains(reg))
//    {
//        QMessageBox::warning(NULL, QString("提示"),
//                             QString("请输入正整数"),
//                             QString("确定"));
//        ui->in_d->setFocus();
//        ui->in_d->clear();
//    }
    TypeCheck(ui->in_d, reg, "正整数");
}


void MainWindow::on_in_row_editingFinished()     //检测输入的行数是不是正整数
{
//    if(!ui->in_row->text().contains(reg))
//    {
//        QMessageBox::warning(NULL, QString("提示"),
//                             QString("请输入正整数"),
//                             QString("确定"));
//        ui->in_row->setFocus();
//        ui->in_row->clear();
//    }
    TypeCheck(ui->in_row, reg, "正整数");
}


void MainWindow::on_in_col_returnPressed()     //检测输入的列数是不是正整数，输入框回车直接触发按钮事件
{
    TypeCheck(ui->in_col, reg, "正整数");
//    if(!ui->in_col->text().contains(reg)){
//        QMessageBox::warning(NULL, QString("提示"),
//                             QString("请输入正整数"),
//                             QString("确定"));
//        ui->in_col->setFocus();
//        ui->in_col->clear();
//    }
//    else
//        on_calib_clicked();
    on_s1loadpic_clicked();
}


void MainWindow::on_s1loadpic_clicked()     //加载标定图片
{
//    list.clear();
//    imgpath = QFileDialog::getExistingDirectory(this, "选择文件夹", "/");
////    if(imgpath.isEmpty()) return;
//    QDir dir(imgpath);
//    QStringList filters;
//    filters<< "*.jpg" << "*.png";
//    QStringList dl = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);
//    if(dl.isEmpty()){
//        QMessageBox::critical(NULL, "错误", "路径下没有图片，请确认是否输入了正确的路径", "确定");
//        return;
//    }
//    for(int i = 0; i < dl.size(); i++){
//        list.push_back(imgpath + "/" + dl[i]);
//    }
//    qDebug()<<list;


//    QString picture1 = list[0];
//    QImage img(picture1);
//    recvShowPicSignal(img, ui->imgview);
    imgpath = QFileDialog::getExistingDirectory(this, "选择文件夹", "/");
    if(imgpath.isEmpty())
        return;

    list = GetImgList(imgpath);
    if(list.isEmpty()){
        QMessageBox::critical(NULL, "错误", "路径下没有图片，请确认是否输入了正确的路径", "确定");
        return;
    }

    QImage img(list[0]);
    recvShowPicSignal(img, ui->imgview);
}


int calib_x, calib_y, calib_d;      //靶标信息
void MainWindow::on_calib_clicked()     //相机标定并展示结果
{
    if(ui->in_col->text().isEmpty() || ui->in_row->text().isEmpty() || ui->in_d->text().isEmpty()){
        QMessageBox::critical(this, "错误", "请先输入靶标信息", "确定");
        return;
    }
    if(imgpath.isEmpty()){
        QMessageBox::critical(NULL, "错误", "请先选择图片路径", "确定");
        return;
    }

    QString path = QDir::currentPath() + "/data/相机标定/";     //删除之前的标定结果
    QDir dir(path);
    if(!dir.isEmpty())
        dir.removeRecursively();
//    list.clear();
//    delete m_Image;
//    ui->imgview->update();

    this->setCursor(Qt::WaitCursor);

    calib_d = ui->in_d->text().toInt();
    calib_x = ui->in_col->text().toInt();
    calib_y = ui->in_row->text().toInt();

    CalibThread *thread2 = new CalibThread();       //标定子线程
    thread2->start();
    connect(thread2, SIGNAL(completed()), this, SLOT(showresult()));
}


void CalibThread::run()
{
    calib(PathWithCHN(imgpath), calib_x, calib_y, calib_d);
    emit completed();       //标定完成信号
}


void MainWindow::showresult()        //展示标定结果
{
    this->setCursor(Qt::ArrowCursor);
    QString path = "./data/相机标定/";
//    QDir dir(path);
//    if(!dir.isEmpty())
//        dir.removeRecursively();
    list.clear();
    delete m_Image;
    ui->imgview->update();

    QFile fk("./data/K.txt");     //内参矩阵保存文件
    QFile fd("./data/distCoeffs.txt");     //畸变系数保存文件
    QFile fe("./data/相机标定/total_err.txt");       //标定平均误差
    QFile error("./data/相机标定/error.txt");
    QFile all("./data/相机标定/all.txt");
    QFile dif("./data/相机标定/diff.txt");

    if(all.exists()){
        QMessageBox::critical(NULL, "错误", "所有图片提取角点均失败，请检查是否输入了正确的靶标信息", "确定");
        return;
    }
    if(dif.exists()){
        QMessageBox::critical(NULL, "错误", "存在不同分辨率的图片，标定失败", "确定");
        return;
    }
    if(error.exists()){
        error.open(QIODevice::ReadOnly);
        QTextStream in(&error);
        QStringList t = in.readAll().replace("\r", "").split("\n");
        error.close();
        ui->text1->setText("提示，以下图片角点提取失败：");
        for(auto & i : t)
            if(!i.isEmpty())
                ui->text1->append(i);
        ui->text1->append("一共" + QString::number(t.size()-1) + "张图片\n");
    }

    list = GetImgList(path);

    fk.open(QIODevice::ReadOnly);
    fd.open(QIODevice::ReadOnly);
    fe.open(QIODevice::ReadOnly);
    QTextStream kin(&fk), din(&fd), ein(&fe);
    ui->text1->append("内参矩阵：");
    QStringList k = kin.readAll().split(" ");
    ui->text1->append(k[0] + " " + k[1] + " " + k[2]);
    ui->text1->append(k[3] + " " + k[4] + " " + k[5]);
    ui->text1->append(k[6] + " " + k[7] + " " + k[8]);
    QStringList dc = din.readAll().split(" ");
    ui->text1->append("径向畸变系数：");
    ui->text1->append(dc[0]+"  "+dc[1]+"  "+dc[4]);
    ui->text1->append("切向畸变系数：");
    ui->text1->append(dc[2]+"  "+dc[3]+"\n");
    ui->text1->append("总体平均误差：" + ein.readAll() + "像素");
    fk.close();
    fd.close();
    fe.close();

    QImage img(list[0]);
    recvShowPicSignal(img, ui->imgview);
}


void MainWindow::on_pushButton_3_clicked()     //显示下一张图片
{
    if(list.size() > 0)
    {
        QPixmap pixmap(ic.NextImage(list));
        recvShowPicSignal(pixmap.toImage(), ui->imgview);
    }
}


void MainWindow::on_pushButton_2_clicked()     //显示上一张图片
{
    if(list.size() > 0)
    {
        QPixmap pixmap(ic.PreImage(list));
        recvShowPicSignal(pixmap.toImage(), ui->imgview);
    }
}


void MainWindow::recvShowPicSignal(QImage image, QGraphicsView *view)     //可以缩放和拖拽地显示图片
{
    QPixmap ConvertPixmap=QPixmap::fromImage(image);
    QGraphicsScene  *qgraphicsScene = new QGraphicsScene;//要用QGraphicsView就必须要有QGraphicsScene搭配着用
    m_Image = new ImageWidget(&ConvertPixmap);//实例化类ImageWidget的对象m_Image，该类继承自QGraphicsItem，是自己写的类
    int nwith = view->width();//获取界面控件Graphics View的宽度
    int nheight = view->height();//获取界面控件Graphics View的高度
    m_Image->setQGraphicsViewWH(nwith,nheight);//将界面控件Graphics View的width和height传进类m_Image中
    qgraphicsScene->addItem(m_Image);//将QGraphicsItem类对象放进QGraphicsScene中
    /*使视窗的大小固定在原始大小，不会随图片的放大而放大（默认状态下图片放大的时候视窗两边会自动出现滚动条，并且视窗内的视野会变大），
        防止图片放大后重新缩小的时候视窗太大而不方便观察图片*/
    view->setSceneRect(QRectF(-(nwith/2),-(nheight/2),nwith,nheight));
    view->setScene(qgraphicsScene);
    view->setFocus();//将界面的焦点设置到当前Graphics View控件
}


/***********************************************************初始解算*****************************************************************/

void MainWindow::add(QLineEdit *in1, QLineEdit *in2, int num)      //特征点信息添加到特征点列表中
{

    QFile f("./data/初始解算/c.txt");
    if(!f.exists()){
        QMessageBox::critical(NULL, "错误", "每个角点需要选取3次", "确定");
        return;
    }
    f.open(QIODevice::ReadOnly);
    QTextStream in(&f);
    QStringList pix = in.readAll().trimmed().split(" ");
    f.close();
    QStringList t;
    t.push_back(in1->text());
    t.push_back(in2->text());
    t.push_back(pix[0]);
    t.push_back(pix[1]);
    pointslist[num] = t;

    QStandardItem *mw = new QStandardItem(pointslist[num][0] + " " + pointslist[num][1]);
    QStandardItem *mp = new QStandardItem(pointslist[num][2] + " " + pointslist[num][3]);
    mw->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    mp->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    model->setItem(num, 0, mw);
    model->setItem(num, 1, mp);

    ui->tableView->setModel(model);
    ui->tableView->show();
    choseflag = false;
}

void MainWindow::on_point_x_editingFinished()
{
    TypeCheck(ui->point_x, reg2, "非负整数");
    if(ui->point_y->text().isEmpty())
        pnum++;
}


void MainWindow::on_point_y_returnPressed()
{
    TypeCheck(ui->point_x, reg2, "非负整数");
    add(ui->point_x, ui->point_y, 0);
    if(pnum >= 6)
        hcalcula();
    else
        ui->point_x_2->setFocus();
}


void MainWindow::on_point_x_2_editingFinished()
{
    TypeCheck(ui->point_x_2, reg2, "非负整数");
    if(ui->point_y_2->text().isEmpty())
        pnum++;
}


void MainWindow::on_point_y_2_returnPressed()
{
    TypeCheck(ui->point_y_2, reg2, "非负整数");
    add(ui->point_x_2, ui->point_y_2, 1);
    if(pnum >= 6)
        hcalcula();
    else
        ui->point_x_3->setFocus();
}

void MainWindow::on_point_x_3_editingFinished()
{
    TypeCheck(ui->point_x_3, reg2, "非负整数");
    if(ui->point_y_3->text().isEmpty())
        pnum++;
}


void MainWindow::on_point_y_3_returnPressed()
{
    TypeCheck(ui->point_y_3, reg2, "非负整数");
    add(ui->point_x_3, ui->point_y_3, 2);
    if(pnum >= 6)
        hcalcula();
    else
        ui->point_x_4->setFocus();
}

void MainWindow::on_point_x_4_editingFinished()
{
    TypeCheck(ui->point_x_4, reg2, "非负整数");
    if(ui->point_y_4->text().isEmpty())
        pnum++;
}


void MainWindow::on_point_y_4_returnPressed()
{
    TypeCheck(ui->point_y_4, reg2, "非负整数");
    add(ui->point_x_4, ui->point_y_4, 3);
    if(pnum >= 6)
        hcalcula();
    else
        ui->point_x_5->setFocus();
}

void MainWindow::on_point_x_5_editingFinished()
{
    TypeCheck(ui->point_x_5, reg2, "非负整数");
    if(ui->point_y_5->text().isEmpty())
        pnum++;
}


void MainWindow::on_point_y_5_returnPressed()
{
    TypeCheck(ui->point_y_5, reg2, "非负整数");
    add(ui->point_x_5, ui->point_y_5, 4);
    if(pnum >= 6)
        hcalcula();
    else
        ui->point_x_6->setFocus();
}


void MainWindow::on_point_x_6_editingFinished()
{
    TypeCheck(ui->point_x_6, reg2, "非负整数");
    if(ui->point_y_6->text().isEmpty())
        pnum++;
}


void MainWindow::on_point_y_6_returnPressed()
{
    TypeCheck(ui->point_y_6, reg2, "非负整数");
    add(ui->point_x_6, ui->point_y_6, 5);
    if(pnum >= 6)
        hcalcula();
}


void MainWindow::on_in_d_2_editingFinished()
{
    TypeCheck(ui->in_d_2, reg3, "正数");
}


QImage MatToQImage(cv::Mat mtx)     //cv::Mat转成QImage
{
    switch (mtx.type()){
    case CV_8UC1:{
            QImage img((const unsigned char *)(mtx.data), mtx.cols, mtx.rows, mtx.cols, QImage::Format_Grayscale8);
            return img;
        }
        break;
    case CV_8UC3:{
            QImage img((const unsigned char *)(mtx.data), mtx.cols, mtx.rows, mtx.cols * 3, QImage::Format_RGB888);
            return img.rgbSwapped();
        }
        break;
    case CV_8UC4:{
            QImage img((const unsigned char *)(mtx.data), mtx.cols, mtx.rows, mtx.cols * 4, QImage::Format_ARGB32);
            return img;
        }
        break;
    default:{
            QImage img;
            return img;
        }
        break;
    }
}


void MainWindow::showpic(QImage pic, QGraphicsView *view)     //可缩放的显示图片，并可以获取点击处像素坐标
{
    ImageItem *it = new ImageItem(QPixmap::fromImage(pic));

    it->setGraphicsViewWH(view->width(), view->height());
    sc->addItem(it);
    view->setSceneRect(QRectF(0, 0, view->width(), view->height()));
    view->setScene(sc);
}

cv::VideoCapture camera;
cv::Mat cframe;
CameraThread *ct;       //相机子线程
void MainWindow::on_loadcamera_clicked()     //step2加载摄像头
{
    videos = 1;
    ui->camera->setChecked(true);
    ui->groupBox_2->show();
    ui->freeze->setEnabled(true);
    ct = new CameraThread();
    ct->start();

    connect(ct, SIGNAL(hide()), this, SLOT(pbhide()));

//    camera.open(0);
//    while(1){
//        camera>>cframe;
//        cv::namedWindow("camera", 0);
//        cv::imshow("camera", cframe);

//        HWND hw = (HWND)cvGetWindowHandle("camera");
//        hw = FindWindow(0, L"camera");
//        ::SetWindowPos(hw, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
//        ::ShowWindow(hw, SW_SHOW);

//        cv::waitKey(10);
//        if(cv::getWindowProperty("camera", cv::WND_PROP_VISIBLE) < 1){
//            cv::destroyAllWindows();
//            break;
//        }
//    }
//    camera.release();
}


void CameraThread::run()
{
    camera.open(cid);
    while(1){
        camera>>cframe;
        cv::namedWindow("camera", 0);
        cv::imshow("camera", cframe);

        HWND hw = (HWND)cvGetWindowHandle("camera");
        hw = FindWindow(0, L"camera");
        ::SetWindowPos(hw, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        ::ShowWindow(hw, SW_SHOW);

        cv::waitKey(10);
        if(cv::getWindowProperty("camera", cv::WND_PROP_VISIBLE) < 1){
            cv::destroyAllWindows();
            emit hide();
            break;
        }
    }
    camera.release();
}


void MainWindow::pbhide()
{
    ui->freeze->setEnabled(false);
}


void MainWindow::on_freeze_clicked()        //截图手动选取初始帧
{
    cv::imwrite(PathWithCHN("./data/初始解算/capture.jpg"), cframe);
    QImage img("./data/初始解算/capture.jpg");
    sc = new ImageScene();
    showpic(img, ui->s2view);
}


void MainWindow::on_chosevideo_clicked()     //step2选择视频文件
{
    videopath = QFileDialog::getOpenFileName(this, "选择文件", "/", "视频文件 (*.mp4 *.avi);;");
    if(videopath.isEmpty())
        return;
    cv::VideoCapture video(PathWithCHN(videopath));

    cv::Mat frame1;
    video.read(frame1);     //获取视频第一帧
    QImage img = MatToQImage(frame1);

//        cv::imwrite("./data/0.png", frame1);
//        piclist.append("./data/0.png");

    sc = new ImageScene();     //使用重写的类来读取图片，实现点击图片获得图片像素坐标
    showpic(img, ui->s2view);
//        ImageItem *it = new ImageItem(QPixmap::fromImage(img));
//        it->setGraphicsViewWH(ui->s2view->width(), ui->s2view->height());
//        sc->addItem(it);
//        ui->s2view->setSceneRect(QRectF(0, 0, ui->s2view->width(), ui->s2view->height()));
//        ui->s2view->setScene(sc);
    videos = 2;
    ui->video->setChecked(true);
}


//void MainWindow::on_s2run_clicked()     //录入按钮，输出选取点的物理坐标和像素坐标
//{
//    QFile f("./data/coordinate.txt");     //读取选取点的像素坐标
//    if(!flag2)
//        QMessageBox::warning(NULL, QString("提示"),
//                             QString("请先选取图片或视频"),
//                             QString("确定"));
//    else if(!f.open(QIODevice::ReadOnly))
//        QMessageBox::warning(NULL, QString("提示"),
//                             QString("每个点需要选择三次"),
//                             QString("确定"));
//    QTextStream in(&f);
//    QString read = in.readAll().trimmed();
//    QStringList pix = read.split(" ");     //获得像素坐标
//    QString pix_x = pix[0];
//    QString pix_y = pix[1];
//    QString wx, wy;
//    f.close();

//    if(pointnum == 0){
//        wx = ui->point_x->text();
//        wy = ui->point_y->text();
//        QStringList t;
//        t.push_back(wx);t.push_back(wy);
//        t.push_back(pix_x);t.push_back(pix_y);
//        pointslist[pointnum] = t;
//        ui->point->setStyleSheet("color: black");
//    }
//    else if(pointnum == 1){
//        wx = ui->point_x_2->text();
//        wy = ui->point_y_2->text();
//        QStringList t;
//        t.push_back(wx);t.push_back(wy);
//        t.push_back(pix_x);t.push_back(pix_y);
//        pointslist[pointnum] = t;
//        ui->point_2->setStyleSheet("color: black");
//    }
//    else if(pointnum == 2){
//        wx = ui->point_x_3->text();
//        wy = ui->point_y_3->text();
//        QStringList t;
//        t.push_back(wx);t.push_back(wy);
//        t.push_back(pix_x);t.push_back(pix_y);
//        pointslist[pointnum] = t;
//        ui->point_3->setStyleSheet("color: black");
//    }
//    else if(pointnum == 3){
//        wx = ui->point_x_4->text();
//        wy = ui->point_y_4->text();
//        QStringList t;
//        t.push_back(wx);t.push_back(wy);
//        t.push_back(pix_x);t.push_back(pix_y);
//        pointslist[pointnum] = t;
//        ui->point_4->setStyleSheet("color: black");
//    }
//    else if(pointnum == 4){
//        wx = ui->point_x_5->text();
//        wy = ui->point_y_5->text();
//        QStringList t;
//        t.push_back(wx);t.push_back(wy);
//        t.push_back(pix_x);t.push_back(pix_y);
//        pointslist[pointnum] = t;
//        ui->point_5->setStyleSheet("color: black");
//    }
//    else if(pointnum == 5){
//        wx = ui->point_x_6->text();
//        wy = ui->point_y_6->text();
//        QStringList t;
//        t.push_back(wx);t.push_back(wy);
//        t.push_back(pix_x);t.push_back(pix_y);
//        pointslist[pointnum] = t;
//        ui->point_6->setStyleSheet("color: black");
//    }

//    for(int i = 0; i < 6; i++){
//        if(pointslist[i][3] != "0"){
//            QStandardItem *w = new QStandardItem(pointslist[i][0] + "\n" + pointslist[i][1]);
//            QStandardItem *p = new QStandardItem(pointslist[i][2] + "\n" + pointslist[i][3]);
//            w->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
//            p->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
//            model->setItem(i, 0, w);
//            model->setItem(i, 1, p);
//        }
//    }

//    ui->tableView->setModel(model);
//    ui->tableView->show();

//    choseflag = false;

////    else{
//////        QList<QGraphicsItem*> listItem = sc->items();     //删除之前三次的标记
//////        for(int i = 0; i < 3; i++)
//////        {
//////            sc->removeItem(listItem.at(0));
//////            listItem.removeAt(0);
//////        }

////        num++;
////        qDebug()<<num;
////        double x = ui->in_x->text().toDouble();
////        double y = ui->in_y->text().toDouble();
////        double d = ui->in_d_2->text().toDouble();

////        QTextStream in(&f);
////        QString read = in.readAll();
////        QStringList pix = read.split(" ");     //获得像素坐标
////        double pix_y = pix[pix.size()-1].toDouble();
////        double pix_x = pix[pix.size()-2].toDouble();
//////        QGraphicsRectItem  *pItem = new QGraphicsRectItem();     //画上一个标记
//////        QPen pen = pItem->pen();
//////        pen.setWidth(5);
//////        pen.setColor(Qt::red);
//////        pItem->setPen(pen);
//////        pItem->setRect(pix_x, pix_y, 2, 2);
//////        sc->addItem(pItem);
////        f.remove();

////        QString str = QString::number(x*d)+" "+QString::number(y*d)+" "+QString::number(pix_x, 'f', 3)
////                +" "+QString::number(pix_y, 'f', 3);
////        ui->s2txt->append(str);

////        sc->clear();
////        QImage pic(piclist[piclist.length()-1]);
////        showpic(pic, ui->s2view);

////        ui->in_x->clear();
////        ui->in_y->clear();
////        ui->in_x->setFocus();
////        choseflag = false;
////        QFile ff("./data/log.txt");     //保存所有已选点
////        if(num == 1 && ff.exists())     //存入第一个点时若已存在此文件，先删除
////            ff.remove();
//////        if(!ff.open(QIODevice::WriteOnly | QIODevice::Append))
//////            QMessageBox::critical(NULL, QString("提示"),
//////                                  QString("打开/data/log.txt文件失败"),
//////                                  QString("确定"));
//////        else{
//////            QTextStream input(&ff);
//////            input<<QString::number(x*d)<<" "<<QString::number(y*d)<<" "<<QString::number(pix_x, 'f', 3)
//////                <<" "<<QString::number(pix_y, 'f', 3)<<"\n";
//////            ff.close();
//////        }
////    }
//}


void ImageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)     //监听鼠标点击事件，点击后获取坐标并做标记
{
    if(event->button() == Qt::LeftButton){
//        double x = event->pos().x();
//        double y = event->pos().y();
        if(choseflag){     //每个点需要选三次，若多余三次需要先录入当前点
            QMessageBox::warning(NULL, "提示", "已选择一个点三次，请先录入该点", "确定");
            return;
        }

//            QGraphicsRectItem  *pItem = new QGraphicsRectItem();     //每次点击都做一个标记
//            QPen pen = pItem->pen();
//            pen.setWidth(2);
//            pen.setColor(Qt::red);
//            pItem->setPen(pen);
//            pItem->setRect(event->scenePos().x(), event->scenePos().y(), 2, 2);
//            sc->addItem(pItem);
        qDebug() << "(" << event->pos().x() << ", " << event->pos().y() << ")";

        QFile f("./data/初始解算/c.txt");
        if(f.exists())
            f.remove();

        plist.push_back(event->pos().x());
        plist.push_back(event->pos().y());

        if(plist.size() == 6){
            double x2, y2;
            x2 = (plist[0] + plist[2] + plist[4]) / 3;
            y2 = (plist[1] + plist[3] + plist[5]) / 3;
            plist.clear();
            QString pc = QString::number(x2, 'f', 3) + " " + QString::number(y2, 'f', 3);
            qDebug()<<pc;
            f.open(QIODevice::WriteOnly);
            QTextStream in(&f);
            in<<pc<<"\n";
            f.close();
            choseflag = true;
        }

//            m[n][0] = event->pos().x();
//            m[n][1] = event->pos().y();
//            n++;
//        if(n == 3){     //已选三次之后计算出均值，用作最终输入的像素坐标
//            n = 0;
//            double x2, y2;
//            x2 = (m[0][0] + m[1][0] + m[2][0]) / 3;
//            y2 = (m[0][1] + m[1][1] + m[2][1]) / 3;
//            qDebug()<<x2<<" "<<y2;
//            QString coordinate = QString::number(x2, 'f', 3) + " " + QString::number(y2, 'f', 3);     //精确到小数点后3位
//            QDir dir;
//            if(!dir.exists("data"))
//                dir.mkdir("data");
//            QFile f("./data/coordinate.txt");
//            f.open(QIODevice::WriteOnly);
//            QTextStream txtOutput(&f);
//            txtOutput << coordinate << "\n";
//            f.close();
//            choseflag = true;

////            int index = piclist.length() - 1;
////            cv::Mat pic = cv::imread(piclist[index].toStdString());
////            circle(pic, cv::Point(x2, y2), 0, cv::Scalar(0, 0, 255), 1);
////            QString picname = "./data/" + QString::number(index+1) + ".png";
////            piclist.append(picname);
////            cv::imwrite(picname.toStdString(), pic);
//        }
    }
    else if(event->button() == Qt::RightButton){     //复原
        m_scaleValue = m_scaleDafault;
        setScale(m_scaleValue);
        setPos(0, 0);
    }
}


//void MainWindow::on_back_clicked()     //撤销选取的点
//{
////    int t = num;
//    num--;
////    QFile f2("./data/log.txt");
//    if(num < 0){
//        QMessageBox::warning(NULL, QString("提示"),
//                             QString("无法撤销"),
//                             QString("确定"));
//        num++;
//    }
////    else if(!f2.open(QIODevice::ReadOnly))
////        QMessageBox::critical(NULL, QString("提示"),
////                              QString("打开/data/log.txt文件失败"),
////                              QString("确定"));
//    else{
//        qDebug()<<num;

//        QList<QGraphicsItem*> listItem = sc->items();
//        sc->removeItem(listItem.at(0));
//        listItem.removeAt(0);
//        QString pfn = piclist[piclist.length()-1];
//        piclist.pop_back();
//        QFile pf(pfn);
//        pf.remove();
//        QString pfn2 = piclist[piclist.length()-1];
//        QImage pic(pfn2);
//        showpic(pic, ui->s2view);

//        ui->s2txt->moveCursor(QTextCursor::End);
//        ui->s2txt->moveCursor(QTextCursor::StartOfLine);
//        ui->s2txt->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
//        ui->s2txt->textCursor().removeSelectedText();
//        ui->s2txt->textCursor().deletePreviousChar();

////        QTextStream in(&f2);
////        f2.open(QIODevice::WriteOnly);
////        QString read = ui->s2txt->toPlainText();
////        in<<read<<"\n";
////        f2.close();


////        int index = 0;
////        QTextStream in(&f2);     //删除最后一行
////        QString read = in.readAll();
////        f2.close();
////        int len = read.length();
////        while(--t)
////        {
////            if(t<0) break;
////            index = read.indexOf('\n', index+1);
////        }
////        read.remove(index, len-index);
////        qDebug()<<read;
////        ui->s2txt->setText(read);
////        f2.open(QIODevice::WriteOnly);
////        in<<read<<"\n";
////        f2.close();
//    }
//}


double MainWindow::x2y(cv::Point2d x, cv::Point2d y)     //求两点之间的距离
{
    double a = pow(x.x - y.x, 2);
    double b = pow(x.y - y.y, 2);
    double ans = sqrt(a + b);
    return ans;
}


bool MainWindow::online(std::vector<cv::Point2d> w)     //判断是否存在三点共线
{
    size_t n = w.size();
    for(int i = 0; i < n; i++)
        for(int j = i+1; j < n; j++)
            for(int k = j+1; k < n; k++){
                double a = x2y(w[i], w[j]);
                double b = x2y(w[i], w[k]);
                double c = x2y(w[j], w[k]);
                double p = (a + b + c)/2;
                double s = sqrt(p*(p-a)*(p-b)*(p-c));     //海伦公式
                if(s == 0){
                    qDebug()<<i<<" "<<j<<" "<<k;
                    return true;
                }
            }
    return false;
}


QString Getfname(QString path)     //获得路径中的文件名
{
    QStringList a = path.split("/");
    QString b = a[a.size()-1];
    QStringList c = b.split(".");
    return c[0];
}

void MainWindow::hcalcula()     //step2解算
{
//    int ans = 0;
//    for(int i : num)
//        ans += i;

    QFileInfo info(videopath);

    if(videos == 1)
        hname = "./data/H.txt";
    else if(videos == 2)
        hname = "./data/H-" + info.baseName() + ".txt";

    if(hname.isEmpty()){
        QMessageBox::critical(NULL, "错误", "请先读取摄像头或读取本地视频", "确定");
        return;
    }

    std::vector<cv::Point2d> w, p;
    for(auto & i : pointslist)
        if(i[3] != "0"){
            w.push_back(cv::Point2d(i[0].toInt(), i[1].toInt()));
            p.push_back(cv::Point2d(i[2].toDouble(), i[3].toDouble()));
        }

//    if(pnum < 6){
//        w.clear();
//        p.clear();
//        QMessageBox::critical(NULL, "错误", "请先完成6个特征点的录入", "确定");
//        return;
//    }

    bool ok = rcs::CalHomographyMatrix(p, w, H);
    if(!ok){
        QMessageBox::critical(NULL, "错误", "单应矩阵H解算失败，请检查是否输入了正确的坐标信息", "确定");
        return;
    }
    QString h1 = QString::number(H(0, 0)) + " " + QString::number(H(0, 1)) + " " + QString::number(H(0, 2));
    QString h2 = QString::number(H(1, 0)) + " " + QString::number(H(1, 1)) + " " + QString::number(H(1, 2));
    QString h3 = QString::number(H(2, 0)) + " " + QString::number(H(2, 1)) + " " + QString::number(H(2, 2));
    QString h = h1 + " " + h2 + " " + h3;

    QFile f(hname);
    QTextStream in(&f);
    f.open(QIODevice::WriteOnly);
    in<<h;
    f.close();

    ui->s2h->setText("H:");
    ui->s2h->append(h1);
    ui->s2h->append(h2);
    ui->s2h->append(h3);
}


/***********************************************************导航定位*****************************************************************/

Eigen::Matrix3d GetMatrix(QString file)     //从txt文件中读取数据存到Eigen::Matrix3d中
{
    Eigen::Matrix3d M;
    QFile f(file);
    QString info = "读取" + file +"文件失败";
    if(!f.open(QIODevice::ReadOnly)){
        QMessageBox::critical(NULL, QString("出错了"), info, QString("确定"));
//        return;
    }
    else{
        QTextStream in(&f);
        QStringList temp = in.readAll().split(" ");
        M<<temp[0].toDouble(),temp[1].toDouble(),temp[2].toDouble(),
           temp[3].toDouble(),temp[4].toDouble(),temp[5].toDouble(),
           temp[6].toDouble(),temp[7].toDouble(),temp[8].toDouble();
    }
    return M;
}


cv::Mat GetMat(QString file, int x, int y)     //从txt文件中读取数据存到cv::Mat中
{
    QFile f(file);
    QString info = "读取" + file +"文件失败";
    cv::Mat_<double> mat(x, y);
    if(!f.open(QIODevice::ReadOnly)){
        QMessageBox::critical(NULL, QString("出错了"), info, QString("确定"));
//        return -1;
    }
    else{
        QTextStream in(&f);
        QStringList df = in.readAll().split(" ");
        for(int i = 0; i < x; i++)
            for(int j = 0; j < y; j++)
                mat.at<double>(i, j) = df[j+i*y].toDouble();
    }
    return std::move(mat);
}


void MainWindow::on_camera_clicked()        //手动选择视频源为实时视频
{
    if(videos == 0){
        QMessageBox::critical(NULL, "错误", "实时视频请先进行初始解算", "确定");
        ui->video->setChecked(true);
        return;
    }
}

bool trackflag = false;
void MainWindow::on_track_clicked()     //step3追踪
{
    if(trackflag){
        QMessageBox::warning(NULL, "错误", "软件目前只能进行一次追踪，再次追踪请重新打开软件", "确定");
        return;
    }

    if(ui->orb->isChecked()){
        ftype = rcs::ORB;
        somename = "-orb";
    }
    else if(ui->surf->isChecked()){
        ftype = rcs::SURF;
        somename = "-surf";
    }
    else /*if(ui->sift->isChecked())*/{
        ftype = rcs::SIFT;
        somename = "-sift";
    }

    if(ui->pnp->isChecked()){
        smethod = rcs::PnP;
        somename += "-pnp";
    }
    else /*if(ui->zhang->isChecked())*/{
        smethod = rcs::Zhang;
        somename += "-zhang";
    }

    if(ui->kcf->isChecked()){
        ttype = rcs::KCF;
        somename += "-kcf";
    }
    else if(ui->boosting->isChecked()){
        ttype = rcs::BOOSTING;
        somename += "-boosting";
    }
    else if(ui->mil->isChecked()){
        ttype = rcs::MIL;
        somename += "-mil";
    }
    else if(ui->tld->isChecked()){
        ttype = rcs::TLD;
        somename += "-tld";
    }
    else /*if(ui->csrt->isChecked())*/{
        ttype = rcs::CSRT;
        somename += "-csrt";
    }

    if(ui->quat->isChecked()){
        somename += "-q";
        tname = "q";
    }
    else if(ui->rmat->isChecked()){
        somename += "-r";
        tname = "r";
    }

    QFileInfo info(videopath);
    if(videos == 0){
        videopath = QFileDialog::getOpenFileName(this, "选择文件", "/", "视频文件(*.mp4 *.avi);;");
        if(videopath.isEmpty())
            return;
        QString ffname = "./data/H-" + info.baseName() + ".txt";
        QFile f(ffname);
        if(!f.exists()){
            QMessageBox::critical(NULL, "错误", "所选视频文件不存在单应矩阵信息，请先进行初始解算", "确定");
            return;
        }
        H = GetMatrix(ffname);
        videos =2;
    }

//    Eigen::Matrix3d K = GetMatrix("./data/K.txt");
//    cv::Mat d = GetMat("./data/distCoeffs.txt", 1, 5);

    ui->s3hide->show();

    trackflag = true;

    pt = new PangolinThread();
    qApp->processEvents();

    pt->start();

    connect(pt, SIGNAL(sendwid()), this, SLOT(showid()));
    connect(pt, SIGNAL(sendata(QString)), this, SLOT(showdata(QString)));
    connect(pt, SIGNAL(sendimg1()), this, SLOT(showimg1()));
    connect(pt, SIGNAL(sendimg2()), this, SLOT(showimg2()));
    connect(pt, SIGNAL(sendlog()), this, SLOT(showlog()));
    connect(pt, SIGNAL(savelog()), this, SLOT(SaveLogFile()));
    connect(pt, SIGNAL(logend(QString)), this, SLOT(showend(QString)));

//    if(videos == 0){
//        if(ui->camera->isChecked()){
////            videos = 1;
////            hname = "./data/H.txt";
//            QMessageBox::critical(NULL, "错误", "实时视频请先完成初始解算单应矩阵", "确定");
//            return;
//        }
//        else if(ui->video->isChecked()){
//            videos = 2;
//            videopath = QFileDialog::getOpenFileName(this, "选择文件", "/", "视频文件 (*.mp4 *.avi *.mkv);; 所有文件 (*.*)");
//            hname = "./data/H-" + Getfname(videopath) + ".txt";
//        }
//        else{
//            QMessageBox::critical(NULL, "错误", "请先选择视频源", "确定");
//            return;
//        }
//    }

//    QFile f1(hname), f2("./data/K.txt"), f3("./data/distCoeffs.txt");
//    if(!f1.open(QIODevice::ReadOnly)){
//        videos = 0;
//        QMessageBox::critical(NULL, "错误", "打开"+hname+"文件失败", "确定");
//        return;
//    }
//    if(!f2.open(QIODevice::ReadOnly)){
//        QMessageBox::critical(NULL, "错误", "打开K.txt文件失败", "确定");
//        return;
//    }
//    if(!f3.open(QIODevice::ReadOnly)){
//        QMessageBox::critical(NULL, "错误", "打开distCoeffs.txt文件失败", "确定");
//        return;
//    }
//    Eigen::Matrix3d H = GetMatrix(hname);
//    Eigen::Matrix3d K = GetMatrix("./data/K.txt");
//    cv::Mat distCoeffs = GetMat("./data/distCoeffs.txt", 1, 5);

//    track(H, K, d, ttype, ftype, smethod);
}


void MainWindow::on_LoadModel_clicked()
{
    modelload = true;
}


void MainWindow::on_ModelUnload_clicked()
{
    modelload = false;
}

using namespace std;
void PangolinThread::run()
{
    cv::VideoCapture video;
    QDateTime cdt = QDateTime::currentDateTime();
    QString date = cdt.toString("-MM-dd-hh-mm");
    QFileInfo info(videopath);

    if(videos == 1){
        logname = "./data/导航定位/camera" + date + "-log.txt";
        resultname = "./data/导航定位/camera" + somename + ".txt";
        video.open(cid);
    }
    else if(videos == 2){
        logname = "./data/导航定位/" + info.baseName() + date + "-log.txt";
        resultname = "./data/导航定位/" + info.baseName() + somename + ".txt";
        video.open(PathWithCHN(videopath));
    }

    QFile result(resultname);

    rcs::myTracker mt(ttype, ftype, smethod);
    Eigen::Matrix3d K = GetMatrix("./data/K.txt");
    cv::Mat d = GetMat("./data/distCoeffs.txt", 1, 5);

    video.read(pglFrame);
    mt.Track(pglFrame, K, d, H, rMat, tVec);
    QString fps = QString::number(mt.frameNum);
    cur_t = tVec.norm();
    emit sendata(fps);

    int fpsnum = 0, endflag = 0;
    double p0[3] = {tVec[0], tVec[1], tVec[2]};

    pangolin::CreateWindowAndBind("pglthread", 1280, 720);
    glEnable(GL_DEPTH_TEST);
    pangolin::OpenGlRenderState s_cam(
                pangolin::ProjectionMatrix(1280, 720, K(0, 0), K(1, 1), K(0, 2), K(1, 2), 0.02, 2000),
                pangolin::ModelViewLookAt(tVec[0], tVec[1], tVec[2], 0, 0, 0, pangolin::AxisY));
    pangolin::Handler3D handler(s_cam);
    pangolin::View& d_cam = pangolin::CreateDisplay().SetBounds(0, 1, 0, 1, -1280.0f/720.0f)
            .SetHandler(&handler);
    pangolin::CreatePanel("menu").SetBounds(0.0, 0.32, 0.0, 0.32);
    pangolin::Var<bool> menu("menu.Trajectory", true, true);
    pangolin::Var<bool> menu2("menu.KeyFrame", true, true);
    std::string pglr1, pglr2, pglr3, pglt;
    pangolin::Var<std::string> pglrm1("menu.R", pglr1);
    pangolin::Var<std::string> pglrm2("menu. ", pglr2);
    pangolin::Var<std::string> pglrm3("menu.  ", pglr3);
    pangolin::Var<std::string> pglvt("menu.t", pglt);

    pglwid = (WId)FindWindow(L"Pangolin", L"pglthread");
    emit sendwid();

    while(1){
        if(pangolin::ShouldQuit())
            break;
        if(video.read(pglFrame)){
            bool ok = mt.Track(pglFrame, K, d, H, rMat, tVec);
            matchimg = MatToQImage(mt.matchImg);
            emit sendimg1();
            emit sendimg2();
            if(ok){
                vector<float> pose_t;
                pose_t.push_back(rMat(0, 0));pose_t.push_back(rMat(1, 0));pose_t.push_back(rMat(2, 0));
                pose_t.push_back(rMat(0, 1));pose_t.push_back(rMat(1, 1));pose_t.push_back(rMat(2, 1));
                pose_t.push_back(rMat(0, 2));pose_t.push_back(rMat(1, 2));pose_t.push_back(rMat(2, 2));
                pose_t.push_back(tVec[0]);pose_t.push_back(tVec[1]);pose_t.push_back(tVec[2]);
                pose.push_back(pose_t);
                pose_t.clear();

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                d_cam.Activate(s_cam);
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

                glBegin(GL_LINES);
                glColor3f(1, 0, 0);
                glVertex3f(p0[0], p0[1], p0[2]);   glVertex3f(p0[0]+10, p0[1], p0[2]);
                glColor3f(0, 1, 0);
                glVertex3f(p0[0], p0[1], p0[2]);   glVertex3f(p0[0], p0[1]+10, p0[2]);
                glColor3f(0, 0 , 1);
                glVertex3f(p0[0], p0[1], p0[2]);   glVertex3f(p0[0], p0[1], p0[2]+10);
                glEnd();

                if(modelload){
                    glShadeModel(GL_SMOOTH);
                    glEnable(GL_LIGHTING);
                    glEnable(GL_LIGHT0);
                    glBegin(GL_TRIANGLES);
                    for(int i = 0; i < normal.size(); i++){
                        int j = 3 * i;
                        glNormal3f(normal[i].x(), normal[i].y(), normal[i].z());
                        glVertex3f(vertex[j].x(), vertex[j].y(), vertex[j].z());
                        glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
                        glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
                    }
                    glEnd();
                    glDisable(GL_LIGHTING);
                }

                int k = 0;
                for(k = 0; k < pose.size();k++){
                    glPushMatrix();
                    std::vector<GLfloat> Twc = { pose[k][0],pose[k][1],pose[k][2],0,
                                              pose[k][3],pose[k][4],pose[k][5],0,
                                              pose[k][6],pose[k][7],pose[k][8],0,
                                              pose[k][9],pose[k][10],pose[k][11],1 };
                    glMultMatrixf(Twc.data());
                    if (menu2) {
                        glLineWidth(2);
                        glBegin(GL_LINES);
                        glColor3f(1.0f, 0.f, 0.f);
                        glVertex3f(0, 0, 0);		glVertex3f(0.1, 0, 0);
                        glColor3f(0.f, 1.0f, 0.f);
                        glVertex3f(0, 0, 0);		glVertex3f(0, 0.2, 0);
                        glColor3f(0.f, 0.f, 1.0f);
                        glVertex3f(0, 0, 0);		glVertex3f(0, 0, 0.1);
                        glColor3f(1.f, 0.f, 1.f);
                        glVertex3f(0, 0, 0);		glVertex3f(0.1, 0.2, 0);
                        glVertex3f(0.1, 0, 0);		glVertex3f(0, 0.2, 0);
                        glVertex3f(0, 0.2, 0);		glVertex3f(0.1, 0.2, 0);
                        glVertex3f(0.1, 0, 0);		glVertex3f(0.1, 0.2, 0);
                        glEnd();
                    }
                    glPopMatrix();
                }

                QString a = QString::number(rMat(0, 0))+","+QString::number(rMat(0, 1))+","+QString::number(rMat(0, 2));
                pglr1 = a.toStdString();
                pglrm1 = pglr1;
                QString b = QString::number(rMat(1, 0))+","+QString::number(rMat(1, 1))+","+QString::number(rMat(1, 2));
                pglr2 = b.toStdString();
                pglrm2 = pglr2;
                QString c = QString::number(rMat(2, 0))+","+QString::number(rMat(2, 1))+","+QString::number(rMat(2, 2));
                pglr3 = c.toStdString();
                pglrm3 = pglr3;
                QString d = QString::number(tVec[0])+','+QString::number(tVec[1])+','+QString::number(tVec[2]);
                pglt = d.toStdString();
                pglvt = pglt;

                if (menu){
                    glLineWidth(2);
                    glBegin(GL_LINES);
                    glColor3f(0.f, 0.f, 0.f);
                    for (int i = 0; i < pose.size() - 1; i++){
                        glVertex3f(pose[i][9], pose[i][10], pose[i][11]);
                        glVertex3f(pose[i + 1][9], pose[i + 1][10], pose[i + 1][11]);
                    }
                    glEnd();
                }
                pangolin::FinishFrame();

                fps = QString::number(mt.frameNum);
                emit sendata(fps);
                QString r = QString::number(rMat(0, 0))+" "+QString::number(rMat(0, 1))+" "+QString::number(rMat(0, 2))+" "+
                        QString::number(rMat(1, 0))+" "+QString::number(rMat(1, 1))+" "+QString::number(rMat(1, 2))+" "+
                        QString::number(rMat(2, 0))+" "+QString::number(rMat(2, 1))+" "+QString::number(rMat(2, 2));
                Eigen::Quaterniond quat(rMat);
                QString q = QString::number(quat.x())+" "+QString::number(quat.y())+" "
                        +QString::number(quat.z())+" "+QString::number(quat.w());
                QString t = QString::number(tVec[0])+" "+QString::number(tVec[1])+" "+QString::number(tVec[2]);
                result.open(QIODevice::WriteOnly | QIODevice::Append);
                QTextStream rin(&result);
                if(tname == "q")
                    rin<<t<<" "<<q<<"\n";
                else if(tname == "r")
                    rin<<t<<" "<<r<<"\n";
                result.close();

                emit savelog();
                fpsnum++;
                endflag = fpsnum - 1;
            }
            else{
                errlog = QString::fromStdString(mt.log);
                emit sendlog();
            }
        }
        else{
            endflag++;
            if(endflag == fpsnum){
                lend = "一共匹配到"+QString::number(fpsnum)+"帧";
                fps = QString::number(fpsnum);
                emit logend(fps);
                emit savelog();
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            d_cam.Activate(s_cam);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            glBegin(GL_LINES);
            glColor3f(1, 0, 0);
            glVertex3f(p0[0], p0[1], p0[2]);   glVertex3f(p0[0]+10, p0[1], p0[2]);
            glColor3f(0, 1, 0);
            glVertex3f(p0[0], p0[1], p0[2]);   glVertex3f(p0[0], p0[1]+10, p0[2]);
            glColor3f(0, 0 , 1);
            glVertex3f(p0[0], p0[1], p0[2]);   glVertex3f(p0[0], p0[1], p0[2]+10);
            glEnd();

            if(modelload){
                glShadeModel(GL_SMOOTH);
                glEnable(GL_LIGHTING);
                glEnable(GL_LIGHT0);
                glBegin(GL_TRIANGLES);
                for(int i = 0; i < normal.size(); i++){
                    int j = 3 * i;
                    glNormal3f(normal[i].x(), normal[i].y(), normal[i].z());
                    glVertex3f(vertex[j].x(), vertex[j].y(), vertex[j].z());
                    glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
                    glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
                }
                glEnd();
                glDisable(GL_LIGHTING);
            }

            int k = 0;
            for(k = 0; k < pose.size();k++){
                glPushMatrix();
                std::vector<GLfloat> Twc = { pose[k][0],pose[k][1],pose[k][2],0,
                                          pose[k][3],pose[k][4],pose[k][5],0,
                                          pose[k][6],pose[k][7],pose[k][8],0,
                                          pose[k][9],pose[k][10],pose[k][11],1 };
                glMultMatrixf(Twc.data());
                if (menu2) {
                    glLineWidth(2);
                    glBegin(GL_LINES);
                    glColor3f(1.0f, 0.f, 0.f);
                    glVertex3f(0, 0, 0);		glVertex3f(0.1, 0, 0);
                    glColor3f(0.f, 1.0f, 0.f);
                    glVertex3f(0, 0, 0);		glVertex3f(0, 0.2, 0);
                    glColor3f(0.f, 0.f, 1.0f);
                    glVertex3f(0, 0, 0);		glVertex3f(0, 0, 0.1);
                    glColor3f(1.f, 0.f, 1.f);
                    glVertex3f(0, 0, 0);		glVertex3f(0.1, 0.2, 0);
                    glVertex3f(0.1, 0, 0);		glVertex3f(0, 0.2, 0);
                    glVertex3f(0, 0.2, 0);		glVertex3f(0.1, 0.2, 0);
                    glVertex3f(0.1, 0, 0);		glVertex3f(0.1, 0.2, 0);
                    glEnd();
                }
                glPopMatrix();
            }

            QString a = QString::number(rMat(0, 0))+","+QString::number(rMat(0, 1))+","+QString::number(rMat(0, 2));
            pglr1 = a.toStdString();
            pglrm1 = pglr1;
            QString b = QString::number(rMat(1, 0))+","+QString::number(rMat(1, 1))+","+QString::number(rMat(1, 2));
            pglr2 = b.toStdString();
            pglrm2 = pglr2;
            QString c = QString::number(rMat(2, 0))+","+QString::number(rMat(2, 1))+","+QString::number(rMat(2, 2));
            pglr3 = c.toStdString();
            pglrm3 = pglr3;
            QString d = QString::number(tVec[0])+','+QString::number(tVec[1])+','+QString::number(tVec[2]);
            pglt = d.toStdString();
            pglvt = pglt;

            if (menu){
                glLineWidth(2);
                glBegin(GL_LINES);
                glColor3f(0.f, 0.f, 0.f);
                for (int i = 0; i < pose.size() - 1; i++){
                    glVertex3f(pose[i][9], pose[i][10], pose[i][11]);
                    glVertex3f(pose[i + 1][9], pose[i + 1][10], pose[i + 1][11]);
                }
                glEnd();
            }
            pangolin::FinishFrame();
        }
    }
}


/*跟踪函数*/
void MainWindow::track(Eigen::Matrix3d H, Eigen::Matrix3d K, cv::Mat distCoeffs,
                       rcs::trackerType ttype, rcs::featureType ftype, rcs::solveMethod smethod)
{
//    ui->modelbox->show();

    cv::VideoCapture video;
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString date = current_date_time.toString("-MM-dd-hh-mm");
//    qDebug()<<date;
    QString logname;
    if(videos == 1){
        logname = "./data/导航定位/camera" + date + "-log.txt";
        video.open(cid);
    }
    else{
        logname = "./data/导航定位/" + Getfname(videopath) + date + "-log.txt";
        qDebug()<<logname;
        video.open(PathWithCHN(videopath));
    }

//    pcl::PointCloud<pcl::PointXYZ>::Ptr head(new pcl::PointCloud<pcl::PointXYZ>);
//    pcl::io::loadPCDFile<pcl::PointXYZ>("head.pcd", *head);
//    if(head->size() == 0){
//        QMessageBox::critical(this, "错误", "模型文件打开失败，请确认模型文件路径是否正确", "确定");
//        return;
//    }

    cv::Mat frame;
    QString fname = "./data/导航定位/" + Getfname(videopath) + somename + ".txt";
    QFile rtf(fname);
    QFile log(logname);

    if(rtf.exists())
        rtf.remove();

    int fpsnum = 0;     //匹配到的帧数
    int endflag = 0;
    double current_t;

    rcs::myTracker track(ttype, ftype, smethod);
    Eigen::Matrix3d r;
    Eigen::Vector3d t;
    video.read(frame);  track.Track(frame, K, distCoeffs, H, r, t);

    current_t = t.norm();
//    qDebug()<<current_t;
    ui->s3log->append("Frame:"+QString::number(track.frameNum));
    ui->s3log->append("R:"+QString::number(r(0,0))+" "+QString::number(r(0,1))+" "+QString::number(r(0,2)));
    ui->s3log->append(QString::number(r(1,0))+" "+QString::number(r(1,1))+" "+QString::number(r(1,2)));
    ui->s3log->append(QString::number(r(2,0))+" "+QString::number(r(2,1))+" "+QString::number(r(2,2)));
    ui->s3log->append("||R||="+QString::number(r.determinant()));
    ui->s3log->append("t:"+QString::number(t[0])+"\n"+QString::number(t[1])+"\n"+QString::number(t[2]));
    ui->s3log->append("t:"+QString::number(t[0])+" "+QString::number(t[1])+" "+QString::number(t[2]));
    ui->s3log->append("||t||="+QString::number(t.norm()));

    int fx, fy, cx, cy;
    fx = K(0, 0);       fy = K(1, 1);
    cx = K(0, 2);       cy = K(1, 2);

    pangolin::CreateWindowAndBind("Pangolin_Track", 1024, 768);     //pangolin初始化
    glEnable(GL_DEPTH_TEST);
    pangolin::OpenGlRenderState s_cam(
        pangolin::ProjectionMatrix(1024, 768, fx, fy, cx, cy, 0.02, 2000),
        pangolin::ModelViewLookAt(t[0], t[1], t[2], 0, 0, 0, pangolin::AxisY)
    );
    pangolin::Handler3D handler(s_cam);
    pangolin::View& d_cam = pangolin::CreateDisplay()
        .SetBounds(0.0, 1.0, 0.0, 1.0, -640.0f / 480.0f)
        .SetHandler(&handler);
    pangolin::CreatePanel("menu").SetBounds(0.0, 0.32, 0.0, 0.32);
    pangolin::Var<bool> menu("menu.Trajectory", true, true);
    pangolin::Var<bool> menu2("menu.KeyFrame", true, true);
    std::string routput1, routput2, routput3, toutput;
    pangolin::Var<std::string> pangolin_rm1("menu.R", routput1);
    pangolin::Var<std::string> pangolin_rm2("menu. ", routput2);
    pangolin::Var<std::string> pangolin_rm3("menu.  ", routput3);
    pangolin::Var<std::string> pangolin_vt("menu.t", toutput);

    WId wid = (WId)FindWindow(L"pangolin", L"Pangolin_Track");     //获得pangolin界面的句柄，将其放入窗口部件中
    qDebug()<<"wid:"<<wid;
    QWindow *mw = QWindow::fromWinId(wid);

//    QWidget *qmw;
//    qmw = QWidget::createWindowContainer(mw, this->ui->widget);
//    qmw->setMinimumSize(ui->widget->width()-2, ui->widget->height()-2);
//    qmw->show();

    QWidget *m_widget = QWidget::createWindowContainer(mw, this, Qt::Widget);
//    vbl->setContentsMargins(1, 1, 1, 1);
//    vbl->addWidget(m_widget);
    ui->verticalLayout->setContentsMargins(1, 1, 1, 1);
    ui->verticalLayout->addWidget(m_widget);

//    t[0] -= 10;
//    t[1] += 10;
//    t[2] -= 500;

    while(1){
        if(pangolin::ShouldQuit())
            break;
        if(video.read(frame)){
            qApp->processEvents();
            Eigen::Matrix3d rMatt;
            Eigen::Vector3d tVect;
            bool ok = track.Track(frame, K, distCoeffs, H, rMat, tVec);

            LabelDisplayMat(ui->video_label, frame);
            LabelDisplayMat(ui->match_label, track.matchImg);
            ui->s3hide->show();
            if (ok){
                  /*pangolin可视化*/
                vector<float> pose_t;
                pose_t.push_back(rMat(0, 0));pose_t.push_back(rMat(1, 0));pose_t.push_back(rMat(2, 0));
                pose_t.push_back(rMat(0, 1));pose_t.push_back(rMat(1, 1));pose_t.push_back(rMat(2, 1));
                pose_t.push_back(rMat(0, 2));pose_t.push_back(rMat(1, 2));pose_t.push_back(rMat(2, 2));
                pose_t.push_back(tVec[0]);pose_t.push_back(tVec[1]);pose_t.push_back(tVec[2]);
//                pose_t.push_back(tVec[0] - 10);pose_t.push_back(tVec[1] + 10);pose_t.push_back(tVec[2] - 500);
                pose.push_back(pose_t);
                pose_t.clear();

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                d_cam.Activate(s_cam);
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

//                glPointSize(2);
//                glBegin(GL_POINTS);
//                for (int i = 0; i < head->size(); i++) {
//                    glColor3f(0, 1, 0);
//                    glVertex3d(head->points[i].x, head->points[i].y, head->points[i].z);
//                }
//                glEnd();

                glBegin(GL_LINES);
                glColor3f(1, 0, 0);
                glVertex3f(t[0], t[1], t[2]);   glVertex3f(t[0]+10, t[1], t[2]);
                glColor3f(0, 1, 0);
                glVertex3f(t[0], t[1], t[2]);   glVertex3f(t[0], t[1]+10, t[2]);
                glColor3f(0, 0 , 1);
                glVertex3f(t[0], t[1], t[2]);   glVertex3f(t[0], t[1], t[2]+10);
                glEnd();

                if(modelload){
                glShadeModel(GL_SMOOTH);
                glEnable(GL_LIGHTING);
                glEnable(GL_LIGHT0);
                glBegin(GL_TRIANGLES);
//                glBegin(GL_TRIANGLE_STRIP);
//                glBegin(GL_POINTS);
//                glPointSize(50.0);
                for(int i = 0; i < normal.size(); i++){
                    int j = 3 * i;
                    glNormal3f(normal[i].x(), normal[i].y(), normal[i].z());
                    glVertex3f(vertex[j].x(), vertex[j].y(), vertex[j].z());
                    glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
                    glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
                }
                glEnd();
                glDisable(GL_LIGHTING);
                }

                int k = 0;
                for(k = 0; k < pose.size();k++){
                    glPushMatrix();
                    std::vector<GLfloat> Twc = { pose[k][0],pose[k][1],pose[k][2],0,
                                              pose[k][3],pose[k][4],pose[k][5],0,
                                              pose[k][6],pose[k][7],pose[k][8],0,
                                              pose[k][9],pose[k][10],pose[k][11],1 };
                    glMultMatrixf(Twc.data());
                    if (menu2) {
                        glLineWidth(2);
                        glBegin(GL_LINES);
                        glColor3f(1.0f, 0.f, 0.f);
                        glVertex3f(0, 0, 0);		glVertex3f(0.1, 0, 0);
                        glColor3f(0.f, 1.0f, 0.f);
                        glVertex3f(0, 0, 0);		glVertex3f(0, 0.2, 0);
                        glColor3f(0.f, 0.f, 1.0f);
                        glVertex3f(0, 0, 0);		glVertex3f(0, 0, 0.1);
                        glColor3f(1.f, 0.f, 1.f);
                        glVertex3f(0, 0, 0);		glVertex3f(0.1, 0.2, 0);
                        glVertex3f(0.1, 0, 0);		glVertex3f(0, 0.2, 0);
                        glVertex3f(0, 0.2, 0);		glVertex3f(0.1, 0.2, 0);
                        glVertex3f(0.1, 0, 0);		glVertex3f(0.1, 0.2, 0);
                        glEnd();
                    }
                    glPopMatrix();
                }

                QString a = QString::number(rMat(0, 0))+","+QString::number(rMat(0, 1))+","+QString::number(rMat(0, 2));
                routput1 = a.toStdString();
                pangolin_rm1 = routput1;
                QString b = QString::number(rMat(1, 0))+","+QString::number(rMat(1, 1))+","+QString::number(rMat(1, 2));
                routput2 = b.toStdString();
                pangolin_rm2 = routput2;
                QString c = QString::number(rMat(2, 0))+","+QString::number(rMat(2, 1))+","+QString::number(rMat(2, 2));
                routput3 = c.toStdString();
                pangolin_rm3 = routput3;
                QString d = QString::number(tVec[0])+','+QString::number(tVec[1])+','+QString::number(tVec[2]);
                toutput = d.toStdString();
                pangolin_vt = toutput;

                if (menu)
                {
                    glLineWidth(2);
                    glBegin(GL_LINES);
                    glColor3f(0.f, 0.f, 0.f);
                    for (int i = 0; i < pose.size() - 1; i++)
                    {
                        glVertex3f(pose[i][9], pose[i][10], pose[i][11]);
                        glVertex3f(pose[i + 1][9], pose[i + 1][10], pose[i + 1][11]);
//                        pangolin::ModelViewLookAt(pose[i][9], pose[i][10], pose[i][11], pose[i + 1][9], pose[i + 1][10], pose[i + 1][11],
//                                pose[i + 1][9]-pose[i][9], pose[i + 1][10]-pose[i][10], pose[i + 1][11]-pose[i][11]);
//                        gluLookAt(pose[i][9], pose[i][10], pose[i][11], pose[i + 1][9], pose[i + 1][10], pose[i + 1][11],
//                                pose[i + 1][9]-pose[i][9], pose[i + 1][10]-pose[i][10], pose[i + 1][11]-pose[i][11]);
                    }
                    glEnd();
                }
                pangolin::FinishFrame();

                  /*保存每一帧解算出的R和t*/
                QString r = QString::number(rMat(0, 0))+" "+QString::number(rMat(0, 1))+" "+QString::number(rMat(0, 2))+" "+
                        QString::number(rMat(1, 0))+" "+QString::number(rMat(1, 1))+" "+QString::number(rMat(1, 2))+" "+
                        QString::number(rMat(2, 0))+" "+QString::number(rMat(2, 1))+" "+QString::number(rMat(2, 2));
                Eigen::Quaterniond quat(rMat);
                QString q = QString::number(quat.x())+" "+QString::number(quat.y())+" "
                        +QString::number(quat.z())+" "+QString::number(quat.w());
                QString t = QString::number(tVec[0])+" "+QString::number(tVec[1])+" "+QString::number(tVec[2]);
                rtf.open(QIODevice::WriteOnly | QIODevice::Append);
                QTextStream rtin(&rtf);
//                rtin<<t<<"\n";
                if(tname == "q")
                    rtin<<t<<" "<<q<<"\n";
                else if(tname == "r")
                    rtin<<t<<" "<<r<<"\n";
                rtf.close();
                QTextStream logout(&log);
                log.open(QIODevice::WriteOnly | QIODevice::Append);
                logout<<ui->s3log->toPlainText();
                log.close();

                cout << "Frame:" << track.frameNum << endl;
                cout << "R:" << rMat << endl;
                cout << "||R||=" << rMat.determinant() << endl;
                cout << "t:" << tVec << endl;
                cout << "||t||=" << tVec.norm() << endl;

                ui->s3log->append("Frame:"+QString::number(track.frameNum));
                ui->s3log->append("R:"+QString::number(rMat(0,0))+" "+QString::number(rMat(0,1))+" "+QString::number(rMat(0,2)));
                ui->s3log->append(QString::number(rMat(1,0))+" "+QString::number(rMat(1,1))+" "+QString::number(rMat(1,2)));
                ui->s3log->append(QString::number(rMat(2,0))+" "+QString::number(rMat(2,1))+" "+QString::number(rMat(2,2)));
                ui->s3log->append("||R||="+QString::number(rMat.determinant()));
                ui->s3log->append("t:"+QString::number(tVec[0])+"\n"+QString::number(tVec[1])+"\n"+QString::number(tVec[2]));
                ui->s3log->append("t:"+QString::number(tVec[0])+" "+QString::number(tVec[1])+" "+QString::number(tVec[2]));
                ui->s3log->append("||t||="+QString::number(tVec.norm()));

                ui->s3log->append("Δt="+QString::number(tVec.norm()-current_t));
                current_t = tVec.norm();

                fpsnum++;
                endflag = fpsnum-1;
            }
            else
                ui->s3log->append(QString::fromStdString(track.log));//输出错误信息
        }
        else{
            endflag++;
            if(endflag == fpsnum){
                video.release();
                ui->s3log->append("一共匹配到"+QString::number(fpsnum)+"帧！");
                QString rtname = "./data/导航定位/"+Getfname(fname)+"-"+QString::number(fpsnum)+".txt";
                QFile::rename(fname, rtname);
                QFile log(logname);
                QTextStream logout(&log);
                log.open(QIODevice::WriteOnly);
                logout<<ui->s3log->toPlainText();
                log.close();
            }

//            QString outputlog = ui->s3log->toPlainText();
//            QFile flog("./data/s3log.txt");
//            flog.open(QIODevice::WriteOnly);
//            QTextStream log(&flog);
//            log<<outputlog;
//            flog.close();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            d_cam.Activate(s_cam);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

//            glPointSize(2);
//            glBegin(GL_POINTS);
//            for (int i = 0; i < head->size(); i++) {
//                glColor3f(0, 1, 0);
//                glVertex3d(head->points[i].x, head->points[i].y, head->points[i].z);
//            }
//            glEnd();

            glBegin(GL_LINES);
            glColor3f(1, 0, 0);
            glVertex3f(t[0], t[1], t[2]);   glVertex3f(t[0]+10, t[1], t[2]);
            glColor3f(0, 1, 0);
            glVertex3f(t[0], t[1], t[2]);   glVertex3f(t[0], t[1]+10, t[2]);
            glColor3f(0, 0 , 1);
            glVertex3f(t[0], t[1], t[2]);   glVertex3f(t[0], t[1], t[2]+10);
            glEnd();

            if(modelload){
            glShadeModel(GL_SMOOTH);
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
            glBegin(GL_TRIANGLES);
//            glBegin(GL_TRIANGLE_STRIP);
//            glColor3f(0.f, 1.f, 0.f);
            for(int i = 0; i < normal.size(); i++){
                int j = 3 * i;
                glNormal3f(normal[i].x(), normal[i].y(), normal[i].z());
                glVertex3f(vertex[j].x(), vertex[j].y(), vertex[j].z());
                glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
                glVertex3f(vertex[++j].x(), vertex[j].y(), vertex[j].z());
            }
            glEnd();
            glDisable(GL_LIGHTING);
            }

            int k = 0;
            for(k = 0; k < pose.size();k++){
                glPushMatrix();
                std::vector<GLfloat> Twc = { pose[k][0],pose[k][1],pose[k][2],0,
                                          pose[k][3],pose[k][4],pose[k][5],0,
                                          pose[k][6],pose[k][7],pose[k][8],0,
                                          pose[k][9],pose[k][10],pose[k][11],1 };
                glMultMatrixf(Twc.data());
                if (menu2) {
                    glLineWidth(2);
                    glBegin(GL_LINES);
                    glColor3f(1.0f, 0.f, 0.f);
                    glVertex3f(0, 0, 0);		glVertex3f(0.1, 0, 0);
                    glColor3f(0.f, 1.0f, 0.f);
                    glVertex3f(0, 0, 0);		glVertex3f(0, 0.2, 0);
                    glColor3f(0.f, 0.f, 1.0f);
                    glVertex3f(0, 0, 0);		glVertex3f(0, 0, 0.1);
                    glColor3f(1.f, 0.f, 1.f);
                    glVertex3f(0, 0, 0);		glVertex3f(0.1, 0.2, 0);
                    glVertex3f(0.1, 0, 0);		glVertex3f(0, 0.2, 0);
                    glVertex3f(0, 0.2, 0);		glVertex3f(0.1, 0.2, 0);
                    glVertex3f(0.1, 0, 0);		glVertex3f(0.1, 0.2, 0);
                    glEnd();
                }
                glPopMatrix();
            }
            if (menu){
                glLineWidth(2);
                glBegin(GL_LINES);
                glColor3f(0.f, 0.f, 0.f);
                for (int i = 0; i < pose.size() - 1; i++){
                    glVertex3f(pose[i][9], pose[i][10], pose[i][11]);
                    glVertex3f(pose[i + 1][9], pose[i + 1][10], pose[i + 1][11]);
                }
                glEnd();
            }
            pangolin::FinishFrame();
        }
    }
}


void MainWindow::LabelDisplayMat(QLabel *label, cv::Mat &mat)     //step3在label上显示cv::Mat图像
{
    cv::Mat Rgb;
    QImage Img;
    if (mat.channels() == 3)//RGB Img
    {
        cv::cvtColor(mat, Rgb, CV_BGR2RGB);//颜色空间转换
        Img = QImage((const uchar*)(Rgb.data), Rgb.cols, Rgb.rows, Rgb.cols * Rgb.channels(), QImage::Format_RGB888);
    }
    else//Gray Img
    {
        Img = QImage((const uchar*)(mat.data), mat.cols, mat.rows, mat.cols*mat.channels(), QImage::Format_Indexed8);
    }
    QPixmap pixmap = QPixmap::fromImage(Img);
    pixmap.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    label->setScaledContents(true);
    label->setPixmap(pixmap);
}


void MainWindow::showid()
{
    QWindow *qwin = QWindow::fromWinId(pglwid);
    QWidget *qwid = QWidget::createWindowContainer(qwin, this);
    ui->verticalLayout->addWidget(qwid);
}


void MainWindow::showdata(QString data)
{
    ui->s3log->append("Frame:" + data);
    ui->s3log->append("R:"+QString::number(rMat(0,0))+" "+QString::number(rMat(0,1))+" "+QString::number(rMat(0,2)));
    ui->s3log->append(QString::number(rMat(1,0))+" "+QString::number(rMat(1,1))+" "+QString::number(rMat(1,2)));
    ui->s3log->append(QString::number(rMat(2,0))+" "+QString::number(rMat(2,1))+" "+QString::number(rMat(2,2)));
    ui->s3log->append("||R||="+QString::number(rMat.determinant()));
    ui->s3log->append("t:"+QString::number(tVec[0])+"\n"+QString::number(tVec[1])+"\n"+QString::number(tVec[2]));
    ui->s3log->append("||t||="+QString::number(tVec.norm()));
    ui->s3log->append("Δt="+QString::number(tVec.norm()-cur_t));
}


void MainWindow::showimg1()
{
    QPixmap pixmap = QPixmap::fromImage(MatToQImage(pglFrame));
    pixmap.scaled(ui->video_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->video_label->setScaledContents(true);
    ui->video_label->setPixmap(pixmap);
}


void MainWindow::showimg2()
{
    QPixmap pixmap = QPixmap::fromImage(matchimg);
    pixmap.scaled(ui->match_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->match_label->setScaledContents(true);
    ui->match_label->setPixmap(pixmap);
}


void MainWindow::showlog()
{
    ui->s3log->append(errlog);
}


void MainWindow::SaveLogFile()
{
    QFile f(logname);
    QTextStream in(&f);
    f.open(QIODevice::WriteOnly);
    in<<ui->s3log->toPlainText();
    f.close();
}


void MainWindow::showend(QString data)
{
    ui->s3log->append(lend);
    QFileInfo f(resultname);
    QString rename = "./data/导航定位/" + f.baseName() + "-" + data + ".txt";
    QFile::rename(resultname, rename);
}


void MainWindow::on_pushButton_clicked()
{
    win2->show();
}


void MainWindow::on_pushButton_4_clicked()
{
    win3->show();
}

