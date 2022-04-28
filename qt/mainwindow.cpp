#include "mainwindow.h"
#include "ui_mainwindow.h"

//#include <pcl/point_types.h>
//#include <pcl/io/pcd_io.h>
//#include <pcl/point_cloud.h>
//#include <pcl/visualization/pcl_visualizer.h>


QRegularExpression reg("^[0-9]*[1-9][0-9]*$");     //正整数正则表达式
QRegularExpression reg2("^\\d+$");     //非负整数正则表达式
QRegularExpression reg3("^\\d+(\\.\\d+)?$");     //正数
bool flag1 = false;     //step1是否完成标定
bool flag2 = false;     //step2是否选取图片
bool choseflag = false;     //step2是否已选择三个点
bool flagtrack = false;     //step3是否开始追踪
bool modelload = false;

int pointnum;       //特征点编号
QList<QStringList> pointslist;      //特征点物理坐标和像素坐标列表
QStandardItemModel* model;
int num[6] = {0};     //step2中选取点的个数
int videos = 0;     //视频源,1:摄像头,2:本地视频

QStringList list;     //step1中切换图片列表
QString imgpath;     //step1选取的路径
QString videopath;     //step2选取的视频路径
QStringList piclist;    //step2中做上标记的图片列表
QString hname;     //解算出的H的文件名
QString rtname;     //追踪匹配到的数据文件名
QString tname;     //保存数据格式：四元数或选择矩阵
ImageScene *sc;     //step2画点的scene
int n = 0;     //step2点数量
double m[3][2];     //step2画出的3个点
std::vector<std::vector<float>> pose;     //保存每帧的r和t
QString ft, sm;     //所选的特征提取和位姿解算算法


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    setFixedSize(this->width(),this->height());
    ui->groupBox_2->hide();
    ui->modelbox->hide();
    ui->s3hide->hide();     //隐藏标签
//    ui->tabWidget->tabBar()->hide();     //隐藏tab
    ui->tabWidget->setCurrentIndex(0);     //默认第一页
    /*第三步默认选择pnp、orb和kcf,四元数*/
    ui->pnp->setChecked(true);
    ui->orb->setChecked(true);
    ui->kcf->setChecked(true);
    ui->quat->setChecked(true);

    pointslist.push_back({"0", "0", "0", "0"});pointslist.push_back({"0", "0", "0", "0"});
    pointslist.push_back({"0", "0", "0", "0"});pointslist.push_back({"0", "0", "0", "0"});
    pointslist.push_back({"0", "0", "0", "0"});pointslist.push_back({"0", "0", "0", "0"});

    model = new QStandardItemModel();
    model->setHorizontalHeaderLabels({"物理坐标", "像素坐标"});
    model->setVerticalHeaderLabels({"特征点1","特征点2","特征点3","特征点4","特征点5","特征点6"});

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->tableView->setModel(model);
    ui->tableView->show();

//  加载模型
//    QFile stl("1.stl");
//    int ok = stl.open(QIODevice::ReadOnly);
//    if(!ok){
//        QMessageBox::critical(this, "错误", "读取模型文件失败", "确定");
//        return;
//    }

//    while(!stl.atEnd()){
//        QString line = stl.readLine().trimmed();
//        QStringList word = line.split(" ", Qt::SkipEmptyParts);
//        if(word[0] == "facet")
//            normal.append(QVector3D(word[2].toFloat(), word[3].toFloat(), word[4].toFloat()));
//        if(word[0] == "vertex")
//            vertex.append(QVector3D(word[1].toFloat(), word[2].toFloat(), word[3].toFloat()));
//    }
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


/////////////////////////////////////////////////////////////step1/////////////////////////////////////////////////////////////
void MainWindow::on_in_d_editingFinished()     //检测输入的边长是不是正整数
{
    if(!ui->in_d->text().contains(reg))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入正整数"),
                             QString("确定"));
        ui->in_d->setFocus();
        ui->in_d->clear();
    }
}


void MainWindow::on_in_row_editingFinished()     //检测输入的行数是不是正整数
{
    if(!ui->in_row->text().contains(reg))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入正整数"),
                             QString("确定"));
        ui->in_row->setFocus();
        ui->in_row->clear();
    }
}


void MainWindow::on_in_col_returnPressed()     //检测输入的列数是不是正整数，输入框回车直接触发按钮事件
{
    if(!ui->in_col->text().contains(reg)){
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入正整数"),
                             QString("确定"));
        ui->in_col->setFocus();
        ui->in_col->clear();
    }
    else
//        on_calib_clicked();
        on_s1loadpic_clicked();
}


void MainWindow::on_s1loadpic_clicked()     //加载标定图片
{
    list.clear();
    imgpath = QFileDialog::getExistingDirectory(this, "选择文件夹", "/");
//    if(imgpath.isEmpty()) return;
    QDir dir(imgpath);
    QStringList filters;
    filters<< "*.jpg" << "*.png";
    QStringList dl = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);
    if(dl.isEmpty()){
        QMessageBox::critical(NULL, "错误", "路径下没有图片，请确认是否输入了正确的路径", "确定");
        return;
    }
    for(int i = 0; i < dl.size(); i++){
        list.push_back(imgpath + "/" + dl[i]);
    }
    qDebug()<<list;


    QString picture1 = list[0];
    QImage img(picture1);
    recvShowPicSignal(img, ui->imgview);
}


void MainWindow::on_calib_clicked()     //相机标定并展示结果
{
//    QDir dir;
//    dir.setPath("./data");
//    dir.removeRecursively();     //删除data文件夹
    if(ui->in_col->text().isEmpty() || ui->in_row->text().isEmpty() || ui->in_d->text().isEmpty()){
        QMessageBox::critical(this, "错误", "请先输入靶标信息", "确定");
        return;
    }

//    if(flag1)
//    {
        ui->text1->clear();     //清空结果
        list.clear();
        delete m_Image;
        ui->imgview->update();
//    }

    QFile f1, f2, f3, f4;     //相机标定各种情况
    f1.setFileName("./data/error.txt");     //存在图片角点提取失败
    f2.setFileName("./data/all.txt");     //所有图片角点提取失败
    f3.setFileName("./data/no.txt");     //不存在图片
    f4.setFileName("./data/diff.txt");     //图片分辨率不同
    QString path = QDir::currentPath()+"/data/";     //删除data文件夹下所有jpg文件
    qDebug()<<path;
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QStringList filter;
    filter << "*.jpg" << "*.png";
    dir.setNameFilters(filter);
    QStringList dl = dir.entryList();
    qDebug()<<dl;
    for(int i = 0; i < dl.size(); i++){
        QFile ft(path+dl[i]);
        ft.remove();
    }
    /*删除data文件夹下所有jpg文件*/
    if(f1.exists()) f1.remove();
    if(f2.exists()) f2.remove();
    if(f3.exists()) f3.remove();
    if(f4.exists()) f4.remove();

    int col = ui->in_col->text().toInt();
    int row = ui->in_row->text().toInt();
    int d= ui->in_d->text().toInt();
//    imgpath = QFileDialog::getExistingDirectory(this, "选择文件夹", "/");
//    if(imgpath.isEmpty()) return;
    QByteArray cdata = imgpath.toLocal8Bit();     //防止中文在QString转std::string时乱码
    calib(std::string(cdata), row, col, d);     //相机标定静态库的方法


    QFile file("./data/K.txt");     //内参矩阵保存文件
    QFile file2("./data/distCoeffs.txt");     //畸变系数保存文件
    QFile file3("./data/total_err.txt");
    if(f1.open(QIODevice::ReadOnly))
    {
        QTextStream ein(&f1);
//        ein.setEncoding(QStringConverter::System);     //防止文件中的中文乱码
        QString eread = ein.readAll().replace("\r", " ");
//        qDebug()<<eread;
        QStringList el = eread.split(" \n");
        ui->text1->setText("提示：以下图片角点提取失败");
//        ui->text1->append(ein.readAll());
        for(int i = 0; i < el.length()-1; i++)
            ui->text1->append(el[i]);
        ui->text1->append("一共" + QString::number(el.length()-1) + "张图片\n");
        f1.close();
//        ui->groupBox->show();     //显示隐藏按钮
    }
    if(f4.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("出错了"),
                              QString("存在不同分辨率的图片，请删除后重新标定"),
                              QString("确定"));
//        flag1 = false;
    }
    else if(f3.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("出错了"),
                              QString("目录下不存在图片文件，请确认是否输入了正确的路径"),
                              QString("确定"));
//        flag1 = false;
    }
    else if(f2.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("出错了"),
                              QString("所有图片提取角点均失败，请检查是否输入了正确的靶标大小"),
                              QString("确定"));
//        flag1 = false;
    }
    else if(!file.open(QIODevice::ReadOnly) || !file2.open(QIODevice::ReadOnly)
            || !file3.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("提示"),
                              QString("读取标定结果失败，请检查是否误删文件"),
                              QString("确定"));
//        flag1 = false;
    }
    else{
        QStringList filters;     //使用过滤器创建标定完成图片列表
        filters << "*.jpg";
        dir.setNameFilters(filters);
        QStringList temp = dir.entryList();
        for(int i = 0; i < temp.size(); i++)
            list.push_back(path + temp[i]);

        QTextStream in(&file);     //展示内参信息
        QTextStream in2(&file2);
        QTextStream in3(&file3);
        ui->text1->append("内参矩阵：");
        QStringList k = in.readAll().split(" ");
        ui->text1->append(k[0]+"  "+k[1]+"  "+k[2]);
        ui->text1->append(k[3]+"  "+k[4]+"  "+k[5]);
        ui->text1->append(k[6]+"  "+k[7]+"  "+k[8]);
        QStringList dc = in2.readAll().split(" ");
        ui->text1->append("径向畸变系数：");
        ui->text1->append(dc[0]+"  "+dc[1]+"  "+dc[4]);
        ui->text1->append("切向畸变系数：");
        ui->text1->append(dc[2]+"  "+dc[3]+"\n");
        in3.setEncoding(QStringConverter::System);
        ui->text1->append(in3.readAll());
        file.close();
        file2.close();
        file3.close();

//        QString imgname("extrinsics.png");
//        QImage image;
//        image.load(imgname);
//        ui->label->setPixmap(QPixmap::fromImage(image));
//        ui->label->setScaledContents(true);

        QString iname(list[0]);
        QImage img;
        img.load(iname);
        recvShowPicSignal(img, ui->imgview);     //展示标定后图片，可以缩放拖拽
//        flag1 = true;
    }
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


/////////////////////////////////////////////////////////////step2/////////////////////////////////////////////////////////////
void MainWindow::on_point_x_editingFinished()
{
    if(!ui->point_x->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_x->setFocus();
        ui->point_x->clear();
    }
}


void MainWindow::on_point_y_returnPressed()
{
    if(!ui->point_y->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_y->setFocus();
        ui->point_y->clear();
    }
    else
        on_s2run_clicked();
}


void MainWindow::on_point_x_2_editingFinished()
{
    if(!ui->point_x_2->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_x_2->setFocus();
        ui->point_x_2->clear();
    }
}


void MainWindow::on_point_y_2_returnPressed()
{
    if(!ui->point_y_2->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_y_2->setFocus();
        ui->point_y_2->clear();
    }
    else
        on_s2run_clicked();
}

void MainWindow::on_point_x_3_editingFinished()
{
    if(!ui->point_x_3->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_x_3->setFocus();
        ui->point_x_3->clear();
    }
}


void MainWindow::on_point_y_3_returnPressed()
{
    if(!ui->point_y_3->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_y_3->setFocus();
        ui->point_y_3->clear();
    }
    else
        on_s2run_clicked();
}

void MainWindow::on_point_x_4_editingFinished()
{
    if(!ui->point_x_4->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_x_4->setFocus();
        ui->point_x_4->clear();
    }
}


void MainWindow::on_point_y_4_returnPressed()
{
    if(!ui->point_y_4->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_y_4->setFocus();
        ui->point_y_4->clear();
    }
    else
        on_s2run_clicked();
}

void MainWindow::on_point_x_5_editingFinished()
{
    if(!ui->point_x_5->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_x_5->setFocus();
        ui->point_x_5->clear();
    }
}


void MainWindow::on_point_y_5_returnPressed()
{
    if(!ui->point_y_5->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_y_5->setFocus();
        ui->point_y_5->clear();
    }
    else
        on_s2run_clicked();
}


void MainWindow::on_point_x_6_editingFinished()
{
    if(!ui->point_x_6->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_x_6->setFocus();
        ui->point_x_6->clear();
    }
}


void MainWindow::on_point_y_6_returnPressed()
{
    if(!ui->point_y_6->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->point_y_6->setFocus();
        ui->point_y_6->clear();
    }
    else
        on_s2run_clicked();
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
void MainWindow::on_loadcamera_clicked()     //step2加载摄像头
{
    videos = 1;
    ui->camera->setChecked(true);
    ui->groupBox_2->show();
    camera.open(0);
    while(1){
        camera>>cframe;
        cv::imshow(" ", cframe);
        int flag = cv::waitKey(10);
        if(flag == 27){
            ui->groupBox_2->hide();
            cv::destroyAllWindows();
            break;
        }
    }
    camera.release();
}


void MainWindow::on_freeze_clicked()        //截图手动选取初始帧
{
    cv::imwrite("./data/capture.jpg", cframe);
    QImage img("./data/capture.jpg");
    sc = new ImageScene();
    showpic(img, ui->s2view);
    flag2 = true;
}


void MainWindow::on_chosevideo_clicked()     //step2选择视频文件
{
    if(!ui->in_d_2->text().contains(reg3)){
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入正数"),
                             QString("确定"));
        ui->in_d_2->setFocus();
        ui->in_d_2->clear();
    }
    else{
        videopath = QFileDialog::getOpenFileName(
                        this, "选择文件",
                        "/",
                        "视频文件 (*.mp4 *.avi *.mkv);; 所有文件 (*.*)");
        if(videopath.isEmpty()) return;
        QByteArray cdata = videopath.toLocal8Bit();
        cv::VideoCapture video = cv::VideoCapture(std::string(cdata));
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
        flag2 = true;
        videos = 2;
        ui->video->setChecked(true);
    }
}


void MainWindow::on_point_clicked()
{
    pointnum = 0;
    ui->point->setStyleSheet("color: red");
    num[pointnum] = 1;
}


void MainWindow::on_point_2_clicked()
{
    pointnum = 1;
    ui->point_2->setStyleSheet("color: red");
    num[pointnum] = 1;
}


void MainWindow::on_point_3_clicked()
{
    pointnum = 2;
    ui->point_3->setStyleSheet("color: red");
    num[pointnum] = 1;
}


void MainWindow::on_point_4_clicked()
{
    pointnum = 3;
    ui->point_4->setStyleSheet("color: red");
    num[pointnum] = 1;
}


void MainWindow::on_point_5_clicked()
{
    pointnum = 4;
    ui->point_5->setStyleSheet("color: red");
    num[pointnum] = 1;
}


void MainWindow::on_point_6_clicked()
{
    pointnum = 5;
    ui->point_6->setStyleSheet("color: red");
    num[pointnum] = 1;
}


void MainWindow::on_s2run_clicked()     //录入按钮，输出选取点的物理坐标和像素坐标
{
    QFile f("./data/coordinate.txt");     //读取选取点的像素坐标
    if(!flag2)
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请先选取图片或视频"),
                             QString("确定"));
    else if(!f.open(QIODevice::ReadOnly))
        QMessageBox::warning(NULL, QString("提示"),
                             QString("每个点需要选择三次"),
                             QString("确定"));
    QTextStream in(&f);
    QString read = in.readAll().trimmed();
    QStringList pix = read.split(" ");     //获得像素坐标
    QString pix_x = pix[0];
    QString pix_y = pix[1];
    QString wx, wy;
    f.close();

    if(pointnum == 0){
        wx = ui->point_x->text();
        wy = ui->point_y->text();
        QStringList t;
        t.push_back(wx);t.push_back(wy);
        t.push_back(pix_x);t.push_back(pix_y);
        pointslist[pointnum] = t;
        ui->point->setStyleSheet("color: black");
    }
    else if(pointnum == 1){
        wx = ui->point_x_2->text();
        wy = ui->point_y_2->text();
        QStringList t;
        t.push_back(wx);t.push_back(wy);
        t.push_back(pix_x);t.push_back(pix_y);
        pointslist[pointnum] = t;
        ui->point_2->setStyleSheet("color: black");
    }
    else if(pointnum == 2){
        wx = ui->point_x_3->text();
        wy = ui->point_y_3->text();
        QStringList t;
        t.push_back(wx);t.push_back(wy);
        t.push_back(pix_x);t.push_back(pix_y);
        pointslist[pointnum] = t;
        ui->point_3->setStyleSheet("color: black");
    }
    else if(pointnum == 3){
        wx = ui->point_x_4->text();
        wy = ui->point_y_4->text();
        QStringList t;
        t.push_back(wx);t.push_back(wy);
        t.push_back(pix_x);t.push_back(pix_y);
        pointslist[pointnum] = t;
        ui->point_4->setStyleSheet("color: black");
    }
    else if(pointnum == 4){
        wx = ui->point_x_5->text();
        wy = ui->point_y_5->text();
        QStringList t;
        t.push_back(wx);t.push_back(wy);
        t.push_back(pix_x);t.push_back(pix_y);
        pointslist[pointnum] = t;
        ui->point_5->setStyleSheet("color: black");
    }
    else if(pointnum == 5){
        wx = ui->point_x_6->text();
        wy = ui->point_y_6->text();
        QStringList t;
        t.push_back(wx);t.push_back(wy);
        t.push_back(pix_x);t.push_back(pix_y);
        pointslist[pointnum] = t;
        ui->point_6->setStyleSheet("color: black");
    }

    for(int i = 0; i < 6; i++){
        if(pointslist[i][3] != "0"){
            QStandardItem *w = new QStandardItem(pointslist[i][0] + "\n" + pointslist[i][1]);
            QStandardItem *p = new QStandardItem(pointslist[i][2] + "\n" + pointslist[i][3]);
            w->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            p->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            model->setItem(i, 0, w);
            model->setItem(i, 1, p);
        }
    }

    ui->tableView->setModel(model);
    ui->tableView->show();

    choseflag = false;

//    else{
////        QList<QGraphicsItem*> listItem = sc->items();     //删除之前三次的标记
////        for(int i = 0; i < 3; i++)
////        {
////            sc->removeItem(listItem.at(0));
////            listItem.removeAt(0);
////        }

//        num++;
//        qDebug()<<num;
//        double x = ui->in_x->text().toDouble();
//        double y = ui->in_y->text().toDouble();
//        double d = ui->in_d_2->text().toDouble();

//        QTextStream in(&f);
//        QString read = in.readAll();
//        QStringList pix = read.split(" ");     //获得像素坐标
//        double pix_y = pix[pix.size()-1].toDouble();
//        double pix_x = pix[pix.size()-2].toDouble();
////        QGraphicsRectItem  *pItem = new QGraphicsRectItem();     //画上一个标记
////        QPen pen = pItem->pen();
////        pen.setWidth(5);
////        pen.setColor(Qt::red);
////        pItem->setPen(pen);
////        pItem->setRect(pix_x, pix_y, 2, 2);
////        sc->addItem(pItem);
//        f.remove();

//        QString str = QString::number(x*d)+" "+QString::number(y*d)+" "+QString::number(pix_x, 'f', 3)
//                +" "+QString::number(pix_y, 'f', 3);
//        ui->s2txt->append(str);

//        sc->clear();
//        QImage pic(piclist[piclist.length()-1]);
//        showpic(pic, ui->s2view);

//        ui->in_x->clear();
//        ui->in_y->clear();
//        ui->in_x->setFocus();
//        choseflag = false;
//        QFile ff("./data/log.txt");     //保存所有已选点
//        if(num == 1 && ff.exists())     //存入第一个点时若已存在此文件，先删除
//            ff.remove();
////        if(!ff.open(QIODevice::WriteOnly | QIODevice::Append))
////            QMessageBox::critical(NULL, QString("提示"),
////                                  QString("打开/data/log.txt文件失败"),
////                                  QString("确定"));
////        else{
////            QTextStream input(&ff);
////            input<<QString::number(x*d)<<" "<<QString::number(y*d)<<" "<<QString::number(pix_x, 'f', 3)
////                <<" "<<QString::number(pix_y, 'f', 3)<<"\n";
////            ff.close();
////        }
//    }
}


void ImageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)     //监听鼠标点击事件，点击后获取坐标并做标记
{
    if(event->button() == Qt::LeftButton){
//        double x = event->pos().x();
//        double y = event->pos().y();
        if(choseflag)     //每个点需要选三次，若多余三次需要先录入当前点
            QMessageBox::warning(NULL, QString("提示"),
                                 QString("已选择一个点三次，请先录入该点"),
                                 QString("确定"));
        else{
//            QGraphicsRectItem  *pItem = new QGraphicsRectItem();     //每次点击都做一个标记
//            QPen pen = pItem->pen();
//            pen.setWidth(2);
//            pen.setColor(Qt::red);
//            pItem->setPen(pen);
//            pItem->setRect(event->scenePos().x(), event->scenePos().y(), 2, 2);
//            sc->addItem(pItem);
            qDebug() << "(" << event->pos().x() << ", " << event->pos().y() << ")";


            QFileInfo fi("./data/coordinate.txt");
            QFile ff("./data/coordinate.txt");
            if(fi.isFile())
                ff.remove();

//            ui_text->setText("succeed");

            m[n][0] = event->pos().x();
            m[n][1] = event->pos().y();
            n++;
        }
        if(n == 3){     //已选三次之后计算出均值，用作最终输入的像素坐标
            n = 0;
            double x2, y2;
            x2 = (m[0][0] + m[1][0] + m[2][0]) / 3;
            y2 = (m[0][1] + m[1][1] + m[2][1]) / 3;
            qDebug()<<x2<<" "<<y2;
            QString coordinate = QString::number(x2, 'f', 3) + " " + QString::number(y2, 'f', 3);     //精确到小数点后3位
            QDir dir;
            if(!dir.exists("data"))
                dir.mkdir("data");
            QFile f("./data/coordinate.txt");
            f.open(QIODevice::WriteOnly);
            QTextStream txtOutput(&f);
            txtOutput << coordinate << "\n";
            f.close();
            choseflag = true;

//            int index = piclist.length() - 1;
//            cv::Mat pic = cv::imread(piclist[index].toStdString());
//            circle(pic, cv::Point(x2, y2), 0, cv::Scalar(0, 0, 255), 1);
//            QString picname = "./data/" + QString::number(index+1) + ".png";
//            piclist.append(picname);
//            cv::imwrite(picname.toStdString(), pic);
        }
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

void MainWindow::on_calcula_clicked()     //step2解算
{
    int ans = 0;
    for(int i : num)
        ans += i;

    if(videos == 1)
        hname = "./data/H.txt";
    else if(videos == 2)
        hname = "./data/H-" + Getfname(videopath) + ".txt";

    QFile f2(hname);

    if(!flag2)
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请先选取图片或视频"),
                             QString("确定"));
    else if(ans != 6)
        QMessageBox::warning(NULL, QString("提示"),
                             QString("需要手动选取6个特征点"),
                             QString("确定"));
    else{
        std::vector<cv::Point2d> w, p;
        for(int i = 0; i < 6; i++){
            cv::Point2d wp, pp;
            wp = cv::Point2d(pointslist[i][0].toInt(), pointslist[i][1].toInt());
            pp = cv::Point2d(pointslist[i][2].toDouble(), pointslist[i][3].toDouble());
            w.push_back(wp);
            p.push_back(pp);
        }
        Eigen::Matrix3d H;
        bool ok = rcs::CalHomographyMatrix(p, w, H);     //解算H
        if(!ok)
            QMessageBox::critical(NULL, QString("错误"),
                                  QString("单应矩阵H解算错误，请检查是否输入了正确的坐标"),
                                  QString("确定"));
        else{
            QString h1 = QString::number(H(0, 0)) + " " + QString::number(H(0, 1)) + " " + QString::number(H(0, 2));
            QString h2 = QString::number(H(1, 0)) + " " + QString::number(H(1, 1)) + " " + QString::number(H(1, 2));
            QString h3 = QString::number(H(2, 0)) + " " + QString::number(H(2, 1)) + " " + QString::number(H(2, 2));
            QString h = h1 + " " + h2 + " " + h3;

            QTextStream in2(&f2);
            f2.open(QIODevice::WriteOnly);
            in2<<h;
            f2.close();

            ui->s2h->append("H:");
            ui->s2h->append(h1);
            ui->s2h->append(h2);
            ui->s2h->append(h3);
        }
    }
}


/////////////////////////////////////////////////////////////step3/////////////////////////////////////////////////////////////
Eigen::Matrix3d MainWindow::GetMatrix(QString file)     //从txt文件中读取数据存到Eigen::Matrix3d中
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


cv::Mat MainWindow::GetMat(QString file, int x, int y)     //从txt文件中读取数据存到cv::Mat中
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


void MainWindow::on_track_clicked()     //step3追踪
{
    rcs::trackerType ttype;
    rcs::featureType ftype;
    rcs::solveMethod smethod;

    if(ui->orb->isChecked())
        {ftype = rcs::ORB; ft = "-orb";}
    if(ui->sift->isChecked())
        {ftype = rcs::SIFT; ft = "-sift";}
    if(ui->surf->isChecked())
        {ftype = rcs::SURF; ft = "-surf";}

    if(ui->kcf->isChecked())
        ttype = rcs::KCF;
    if(ui->boosting->isChecked())
        ttype = rcs::BOOSTING;
    if(ui->csrt->isChecked())
        ttype = rcs::CSRT;
    if(ui->mil->isChecked())
        ttype = rcs::MIL;
    if(ui->tld->isChecked())
        ttype = rcs::TLD;
    if(ui->medianflow->isChecked())
        ttype = rcs::MEDIANFLOW;

    if(ui->pnp->isChecked())
        {smethod = rcs::PnP; sm = "-pnp";}
    else
        {smethod = rcs::Zhang; sm = "-zhang";}

    if(ui->quat->isChecked())
        tname = "q";
    else if(ui->rmat->isChecked())
        tname = "r";

    if(videos == 0){
        if(ui->camera->isChecked()){
//            videos = 1;
//            hname = "./data/H.txt";
            QMessageBox::critical(NULL, "错误", "实时视频请先完成初始解算单应矩阵", "确定");
            return;
        }
        else if(ui->video->isChecked()){
            videos = 2;
            videopath = QFileDialog::getOpenFileName(this, "选择文件", "/", "视频文件 (*.mp4 *.avi *.mkv);; 所有文件 (*.*)");
            hname = "./data/H-" + Getfname(videopath) + ".txt";
        }
        else{
            QMessageBox::critical(NULL, "错误", "请先选择视频源", "确定");
            return;
        }
    }

    QFile f1(hname), f2("./data/K.txt"), f3("./data/distCoeffs.txt");
    if(!f1.open(QIODevice::ReadOnly)){
        videos = 0;
        QMessageBox::critical(NULL, "错误", "打开"+hname+"文件失败", "确定");
        return;
    }
    if(!f2.open(QIODevice::ReadOnly)){
        QMessageBox::critical(NULL, "错误", "打开K.txt文件失败", "确定");
        return;
    }
    if(!f3.open(QIODevice::ReadOnly)){
        QMessageBox::critical(NULL, "错误", "打开distCoeffs.txt文件失败", "确定");
        return;
    }

    Eigen::Matrix3d H = GetMatrix(hname);
    Eigen::Matrix3d K = GetMatrix("./data/K.txt");
    cv::Mat distCoeffs = GetMat("./data/distCoeffs.txt", 1, 5);

    track(H, K, distCoeffs, ttype, ftype, smethod);

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
/*跟踪函数*/
void MainWindow::track(Eigen::Matrix3d H, Eigen::Matrix3d K, cv::Mat distCoeffs,
                       rcs::trackerType ttype, rcs::featureType ftype, rcs::solveMethod smethod)
{
//    ui->modelbox->show();

    cv::VideoCapture video;

    if(videos == 1)
        video.open(0);
    else{
        QByteArray cdata = videopath.toLocal8Bit();
        video = cv::VideoCapture(std::string(cdata));
    }

//    pcl::PointCloud<pcl::PointXYZ>::Ptr head(new pcl::PointCloud<pcl::PointXYZ>);
//    pcl::io::loadPCDFile<pcl::PointXYZ>("head.pcd", *head);
//    if(head->size() == 0){
//        QMessageBox::critical(this, "错误", "模型文件打开失败，请确认模型文件路径是否正确", "确定");
//        return;
//    }

    cv::Mat frame;
    QString fname = "./data/"+Getfname(videopath)+ft+sm+"-"+tname+".txt";
    QFile rtf(fname);

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
    string routput1, routput2, routput3, toutput;
    pangolin::Var<string> pangolin_rm1("menu.R", routput1);
    pangolin::Var<string> pangolin_rm2("menu. ", routput2);
    pangolin::Var<string> pangolin_rm3("menu.  ", routput3);
    pangolin::Var<string> pangolin_vt("menu.t", toutput);

    WId wid = (WId)FindWindow(L"pangolin", L"Pangolin_Track");     //获得pangolin界面的句柄，将其放入窗口部件中
    qDebug()<<"wid:"<<wid;
    QWindow *mw = QWindow::fromWinId(wid);

//    QWidget *qmw;
//    qmw = QWidget::createWindowContainer(mw, this->ui->widget);
//    qmw->setMinimumSize(ui->widget->width()-2, ui->widget->height()-2);
//    qmw->show();

    QWidget *m_widget = QWidget::createWindowContainer(mw, this, Qt::Widget);
    ui->verticalLayout->setContentsMargins(1, 1, 1, 1);
    ui->verticalLayout->addWidget(m_widget);

    flagtrack = true;

    while(1){
        if(pangolin::ShouldQuit())
            break;
        if(video.read(frame)){
            qApp->processEvents();
            Eigen::Matrix3d rMat;   Eigen::Vector3d tVec;
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
                rtname = "./data/"+Getfname(fname)+"-"+QString::number(fpsnum)+".txt";
                QFile::rename(fname, rtname);
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


