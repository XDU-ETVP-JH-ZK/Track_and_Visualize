#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(this->width(),this->height());
    ui->groupBox->hide();     //隐藏按钮
    ui->s3hide->hide();     //隐藏标签
    ui->tabWidget->tabBar()->hide();     //隐藏tab
//    ui->tabWidget->setCurrentIndex(0);     //默认第一页
    /*第三步默认选择pnp、orb和kcf,四元数*/
    ui->pnp->setChecked(true);
    ui->orb->setChecked(true);
    ui->kcf->setChecked(true);
    ui->quat->setChecked(true);
}


MainWindow::~MainWindow()
{
    delete ui;
}


QRegularExpression reg("^[0-9]*[1-9][0-9]*$");     //正整数正则表达式
QRegularExpression reg2("^\\d+$");     //非负整数正则表达式
QRegularExpression reg3("^\\d+(\\.\\d+)?$");     //正数
bool flag1 = false;     //step1是否完成标定
bool flag2 = false;     //step2是否选取图片
bool choseflag = false;     //step2是否已选择三个点
bool flagtrack = false;     //step3是否开始追踪
int num = 0;     //step2中选取点的个数
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
        on_calib_clicked();
}


void MainWindow::on_calib_clicked()     //相机标定并展示结果
{
//    QDir dir;
//    dir.setPath("./data");
//    dir.removeRecursively();     //删除data文件夹
    if(flag1)
    {
        ui->text1->clear();     //清空结果
        list.clear();
        delete m_Image;
        ui->imgview->update();
    }

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
    filter << "*.jpg";
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
    imgpath = QFileDialog::getExistingDirectory(this, "选择文件夹", "/");
    QByteArray cdata = imgpath.toLocal8Bit();     //防止中文在QString转std::string时乱码
    calib(std::string(cdata), row, col, d);     //相机标定静态库的方法


    QFile file("./data/K.txt");     //内参矩阵保存文件
    QFile file2("./data/distCoeffs.txt");     //畸变系数保存文件
    if(f4.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("出错了"),
                              QString("存在不同分辨率的图片，请删除后重新标定"),
                              QString("确定"));
        flag1 = false;
    }
    else if(f3.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("出错了"),
                              QString("目录下不存在图片文件，请确认是否输入了正确的路径"),
                              QString("确定"));
        flag1 = false;
    }
    else if(f2.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("出错了"),
                              QString("所有图片提取角点均失败，请检查是否输入了正确的靶标大小"),
                              QString("确定"));
        flag1 = false;
    }
    else if(f1.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream ein(&f1);
        ein.setEncoding(QStringConverter::System);     //防止文件中的中文乱码
        ui->text1->setText("提示：以下图片角点提取失败，请删除后重新标定！");
        ui->text1->append(ein.readAll());
        f1.close();
        ui->groupBox->show();     //显示隐藏按钮
    }
    else if(!file.open(QIODevice::ReadOnly | QIODevice::Text) || !file2.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(NULL, QString("提示"),
                              QString("读取标定结果失败，请检查是否误删文件"),
                              QString("确定"));
        flag1 = false;
    }
    else{
        QStringList filters;     //使用过滤器创建标定完成图片列表
        filters << "*.jpg";
        dir.setNameFilters(filters);
        list = dir.entryList();

        QTextStream in(&file);     //展示内参信息
        QTextStream in2(&file2);
        ui->text1->append("内参矩阵：");
        QStringList k = in.readAll().split(" ");
        ui->text1->append(k[0]+"  "+k[1]+"  "+k[2]);
        ui->text1->append(k[3]+"  "+k[4]+"  "+k[5]);
        ui->text1->append(k[6]+"  "+k[7]+"  "+k[8]);
        QStringList dc = in2.readAll().split(" ");
        ui->text1->append("径向畸变系数：");
        ui->text1->append(dc[0]+"  "+dc[1]+"  "+dc[4]);
        ui->text1->append("切向畸变系数：");
        ui->text1->append(dc[2]+"  "+dc[3]);
        file.close();
        file2.close();

//        QString imgname("extrinsics.png");
//        QImage image;
//        image.load(imgname);
//        ui->label->setPixmap(QPixmap::fromImage(image));
//        ui->label->setScaledContents(true);

        QString iname("./data/dispicture001.jpg");
        QImage img;
        img.load(iname);
        recvShowPicSignal(img, ui->imgview);     //展示标定后图片，可以缩放拖拽
        flag1 = true;
    }
}


void MainWindow::on_hidetest_clicked()     //隐藏按钮，有部分图片角点提取失败，仍然显示标定结果
{
    QString path = QDir::currentPath() + "/data/";
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QStringList filters;
    filters << "*.jpg";
    dir.setNameFilters(filters);
    list = dir.entryList();

    QFile file("./data/K.txt");     //读取标定结果
    QFile file2("./data/distCoeffs.txt");     //读取标定结果
    if(!file.open(QIODevice::ReadOnly) || !file2.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, QString("提示"),
                              QString("读取标定结果失败，请检查是否误删文件"),
                              QString("确定"));
        flag1 = false;
    }
    else{
        QStringList filters;     //使用过滤器创建标定完成图片列表
        filters << "*.jpg";
        dir.setNameFilters(filters);
        list = dir.entryList();

        QTextStream in(&file);     //展示内参信息
        QTextStream in2(&file2);
        ui->text1->append("内参矩阵：");
        QStringList k = in.readAll().split(" ");
        ui->text1->append(k[0]+"  "+k[1]+"  "+k[2]);
        ui->text1->append(k[3]+"  "+k[4]+"  "+k[5]);
        ui->text1->append(k[6]+"  "+k[7]+"  "+k[8]);
        QStringList dc = in2.readAll().split(" ");
        ui->text1->append("径向畸变系数：");
        ui->text1->append(dc[0]+"  "+dc[1]+"  "+dc[4]);
        ui->text1->append("切向畸变系数：");
        ui->text1->append(dc[2]+"  "+dc[3]);
        file.close();
        file2.close();

        QString iname("./data/dispicture001.jpg");
        QImage img;
        img.load(iname);
        recvShowPicSignal(img, ui->imgview);     //展示标定后图片，可以缩放拖拽
        flag1 = true;
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


void MainWindow::on_one2two_clicked()     //step1到step2
{
    if(flag1)
        ui->tabWidget->setCurrentIndex(1);
    else
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请先运行标定程序再进入下一步"),
                             QString("确定"));
}


/////////////////////////////////////////////////////////////step2/////////////////////////////////////////////////////////////
void MainWindow::on_in_x_editingFinished()     //检测输入的是不是非负整数
{
    if(!ui->in_x->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入非负整数"),
                             QString("确定"));
        ui->in_x->setFocus();
        ui->in_x->clear();
    }
}


void MainWindow::on_in_y_returnPressed()     //输入框回车直接触发按钮事件
{
    if(!ui->in_y->text().contains(reg2))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入整数"),
                             QString("确定"));
        ui->in_y->setFocus();
        ui->in_y->clear();
    }
    else
        on_s2run_clicked();
}


void MainWindow::showpic(QImage pic, QGraphicsView *view)     //可缩放的显示图片，并可以获取点击处像素坐标
{
    ImageItem *it = new ImageItem(QPixmap::fromImage(pic));
    it->setGraphicsViewWH(view->width(), view->height());
    sc->addItem(it);
    view->setSceneRect(QRectF(0, 0, view->width(), view->height()));
    view->setScene(sc);
}


void MainWindow::on_chosepic_clicked()     //step2选择图片
{
    if(!ui->in_d_2->text().contains(reg3)){
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请输入正数"),
                             QString("确定"));
        ui->in_d_2->setFocus();
        ui->in_d_2->clear();
    }
    else{
        QString path;
        if(imgpath.isEmpty())
            path = "/";
        else
            path = imgpath;     //把step1选取的路径作为默认路径
        QString s = QFileDialog::getOpenFileName(
                        this, "选择文件",
                        path,
                        "图片文件 (*.jpg *.png);; 所有文件 (*.*)");
        QImage img(s);
        piclist.append(s);
        sc = new ImageScene();     //使用重写的类来读取图片，实现点击图片获得图片像素坐标
        showpic(img, ui->s2view);
        flag2 = true;
    }
}


QImage MatToQImage(cv::Mat mtx)     //cv::Mat转成QImage
{
    switch (mtx.type())
    {
    case CV_8UC1:
        {
            QImage img((const unsigned char *)(mtx.data), mtx.cols, mtx.rows, mtx.cols, QImage::Format_Grayscale8);
            return img;
        }
        break;
    case CV_8UC3:
        {
            QImage img((const unsigned char *)(mtx.data), mtx.cols, mtx.rows, mtx.cols * 3, QImage::Format_RGB888);
            return img.rgbSwapped();
        }
        break;
    case CV_8UC4:
        {
            QImage img((const unsigned char *)(mtx.data), mtx.cols, mtx.rows, mtx.cols * 4, QImage::Format_ARGB32);
            return img;
        }
        break;
    default:
        {
            QImage img;
            return img;
        }
        break;
    }
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
        QByteArray cdata = videopath.toLocal8Bit();
        cv::VideoCapture video = cv::VideoCapture(std::string(cdata));
        cv::Mat frame1;
        video.read(frame1);     //获取视频第一帧
        QImage img = MatToQImage(frame1);

        cv::imwrite("./data/0.png", frame1);
        piclist.append("./data/0.png");

        sc = new ImageScene();     //使用重写的类来读取图片，实现点击图片获得图片像素坐标
        showpic(img, ui->s2view);
//        ImageItem *it = new ImageItem(QPixmap::fromImage(img));
//        it->setGraphicsViewWH(ui->s2view->width(), ui->s2view->height());
//        sc->addItem(it);
//        ui->s2view->setSceneRect(QRectF(0, 0, ui->s2view->width(), ui->s2view->height()));
//        ui->s2view->setScene(sc);
        flag2 = true;
    }
}


void MainWindow::on_s2run_clicked()     //录入按钮，输出选取点的物理坐标和像素坐标
{
    QFile f("./data/coordinate.txt");     //读取选取点的像素坐标
    if(!flag2)
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请先选取图片或视频"),
                             QString("确定"));
    else if(!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(NULL, QString("提示"),
                             QString("每个点需要选择三次"),
                             QString("确定"));
        if(ui->in_x->text().isEmpty())
            ui->in_x->setFocus();
        else
            ui->in_y->setFocus();
    }
    else{
//        QList<QGraphicsItem*> listItem = sc->items();     //删除之前三次的标记
//        for(int i = 0; i < 3; i++)
//        {
//            sc->removeItem(listItem.at(0));
//            listItem.removeAt(0);
//        }

        num++;
        qDebug()<<num;
        double x = ui->in_x->text().toDouble();
        double y = ui->in_y->text().toDouble();
        double d = ui->in_d_2->text().toDouble();

        QTextStream in(&f);
        QString read = in.readAll();
        QStringList pix = read.split(" ");     //获得像素坐标
        double pix_y = pix[pix.size()-1].toDouble();
        double pix_x = pix[pix.size()-2].toDouble();
//        QGraphicsRectItem  *pItem = new QGraphicsRectItem();     //画上一个标记
//        QPen pen = pItem->pen();
//        pen.setWidth(5);
//        pen.setColor(Qt::red);
//        pItem->setPen(pen);
//        pItem->setRect(pix_x, pix_y, 2, 2);
//        sc->addItem(pItem);
        f.remove();

        QString str = QString::number(x*d)+" "+QString::number(y*d)+" "+QString::number(pix_x, 'f', 3)
                +" "+QString::number(pix_y, 'f', 3);
        ui->s2txt->append(str);
        QFile ff("./data/log.txt");     //保存所有已选点
        if(num == 1 && ff.exists())     //存入第一个点时若已存在此文件，先删除
            ff.remove();
        if(!ff.open(QIODevice::WriteOnly | QIODevice::Append))
            QMessageBox::critical(NULL, QString("提示"),
                                  QString("打开/data/log.txt文件失败"),
                                  QString("确定"));
        else{
            QTextStream input(&ff);
            input<<QString::number(x*d)<<" "<<QString::number(y*d)<<" "<<QString::number(pix_x, 'f', 3)
                <<" "<<QString::number(pix_y, 'f', 3)<<"\n";
            ff.close();

            ui->in_x->clear();
            ui->in_y->clear();
            ui->in_x->setFocus();
            choseflag = false;

            sc->clear();
            QImage pic(piclist[piclist.length()-1]);
            showpic(pic, ui->s2view);
        }
    }
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
            QString coordinate = QString::number(x2, 'f', 3) + " " + QString::number(y2, 'f', 3);     //精确到小数点后6位
            QDir dir;
            if(!dir.exists("data"))
                dir.mkdir("data");
            QFile f("./data/coordinate.txt");
            f.open(QIODevice::WriteOnly);
            QTextStream txtOutput(&f);
            txtOutput << coordinate << "\n";
            f.close();
            choseflag = true;

            int index = piclist.length() - 1;
            cv::Mat pic = cv::imread(piclist[index].toStdString());
            circle(pic, cv::Point(x2, y2), 0, cv::Scalar(0, 0, 255), 1);
            QString picname = "./data/" + QString::number(index+1) + ".png";
            piclist.append(picname);
            cv::imwrite(picname.toStdString(), pic);
        }
    }
    else if(event->button() == Qt::RightButton){     //复原
        m_scaleValue = m_scaleDafault;
        setScale(m_scaleValue);
        setPos(0, 0);
    }
}


void MainWindow::on_back_clicked()     //撤销选取的点
{
    int t = num;
    num--;
    QFile f2("./data/log.txt");
    if(num < 0){
        QMessageBox::warning(NULL, QString("提示"),
                             QString("无法撤销"),
                             QString("确定"));
        num++;
    }
    else if(!f2.open(QIODevice::ReadOnly))
        QMessageBox::critical(NULL, QString("提示"),
                              QString("打开/data/log.txt文件失败"),
                              QString("确定"));
    else{
        QList<QGraphicsItem*> listItem = sc->items();
        sc->removeItem(listItem.at(0));
        listItem.removeAt(0);
        QString pfn = piclist[piclist.length()-1];
        piclist.pop_back();
        QFile pf(pfn);
        pf.remove();
        QString pfn2 = piclist[piclist.length()-1];
        QImage pic(pfn2);
        showpic(pic, ui->s2view);

        qDebug()<<num;
        int index = 0;
        QTextStream in(&f2);     //删除最后一行
        QString read = in.readAll();
//        qDebug()<<read;
        f2.close();
        int len = read.length();
        while(--t)
        {
            if(t<0) break;
            index = read.indexOf('\n', index+1);
        }
        read.remove(index, len-index);
        qDebug()<<read;

        ui->s2txt->setText(read);
        f2.open(QIODevice::WriteOnly);
//        QTextStream out(&f2);
//        out<<read<<"\n";
        in<<read<<"\n";
        f2.close();
    }
}


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
    QFile f("./data/log.txt");
    QString fn;
    if(imgpath.isEmpty())
        fn = Getfname(videopath);
    else if(videopath.isEmpty())
        fn = Getfname(imgpath);
    hname = "./data/H.txt";
    QFile f2(hname);

    if(!flag2)
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请先选取图片或视频"),
                             QString("确定"));
    else if(num < 6)
        QMessageBox::warning(NULL, QString("提示"),
                             QString("选的点必须不少于6个"),
                             QString("确定"));
    else if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
            QMessageBox::critical(NULL, QString("提示"),
                                 QString("打开./data/log.txt文件失败"),
                                 QString("确定"));
    else{
        QTextStream in(&f);     //读取坐标数据
        QString a = in.readAll();
        a.replace("\n", " ");
        QStringList b = a.split(" ");
        b.pop_back();
        std::vector<cv::Point2d> w, p;
        for(int i = 0; i< b.size(); i++)
        {
            cv::Point2d p1, p2;
            p1.x = b[i++].toDouble();
            p1.y = b[i++].toDouble();
            p2.x = b[i++].toDouble();
            p2.y = b[i].toDouble();
            w.push_back(p1);
            p.push_back(p2);
        }
//        bool isonline = online(w);
//        if(isonline)
//            QMessageBox::warning(NULL, QString("提示"),
//                                 QString("请尽量不要选取三点共线的点"),
//                                 QString("确定"));

        Eigen::Matrix3d H;
//        rcs::CalHomographyMatrix(w, p, H);
        bool ok = rcs::CalHomographyMatrix(p, w, H);     //解算H
        if(!ok)
            QMessageBox::critical(NULL, QString("错误"),
                                  QString("单应矩阵H解算错误，请检查是否输入了正确的坐标"),
                                  QString("确定"));
        else{
            rcs::validHCalculation(hname.toStdString(), H);     //保存H
            if(!f2.open(QIODevice::ReadOnly | QIODevice::Text))
                QMessageBox::critical(NULL, QString("提示"),
                                     QString("打开"+hname+"文件失败"),
                                     QString("确定"));
            else{
                QTextStream in2(&f2);
                QStringList a = in2.readAll().split(" ");
                QString a1 = a[0] +" "+ a[1] +" "+ a[2];
                QString a2 = a[3] +" "+ a[4] +" "+ a[5];
                QString a3 = a[6] +" "+ a[7] +" "+ a[8];
                ui->s2txt->append("\nH:");
                ui->s2txt->append(a1);
                ui->s2txt->append(a2);
                ui->s2txt->append(a3);
            }
        }
    }
}


void MainWindow::on_two2one_clicked()     //step2到step1
{
    ui->tabWidget->setCurrentIndex(0);
}


void MainWindow::on_two2three_clicked()     //step2到step3
{
    if(num>=4)
        ui->tabWidget->setCurrentIndex(2);
    else
        QMessageBox::warning(NULL, QString("提示"),
                             QString("选的点必须不少于4个"),
                             QString("确定"));
}


/////////////////////////////////////////////////////////////step3/////////////////////////////////////////////////////////////
Eigen::Matrix3d MainWindow::GetMatrix(QString file)     //从txt文件中读取数据存到Eigen::Matrix3d中
{
    Eigen::Matrix3d M;
    QFile f(file);
    QString info = "读取" + file +"文件失败";
    if(!f.open(QIODevice::ReadOnly))
        QMessageBox::critical(NULL, QString("出错了"), info, QString("确定"));
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
    if(!f.open(QIODevice::ReadOnly))
        QMessageBox::critical(NULL, QString("出错了"), info, QString("确定"));
    else{
        QTextStream in(&f);
        QStringList df = in.readAll().split(" ");
        for(int i = 0; i < x; i++)
            for(int j = 0; j < y; j++)
                mat.at<double>(i, j) = df[j+i*y].toDouble();
    }
    return std::move(mat);
}


using namespace std;
/*跟踪函数*/
void MainWindow::track(Eigen::Matrix3d H, Eigen::Matrix3d K, cv::Mat distCoeffs,
                       rcs::trackerType ttype, rcs::featureType ftype, rcs::solveMethod smethod)
{
    if(videopath.isEmpty())
        videopath = QFileDialog::getOpenFileName(this, "选择文件", "/", "视频文件 (*.mp4 *.avi *.mkv);; 所有文件 (*.*);; ");
    QByteArray cdata = videopath.toLocal8Bit();
    cv::VideoCapture video = cv::VideoCapture(std::string(cdata));
    cv::Mat frame;


    QString fname = "./data/"+Getfname(videopath)+ft+sm+".txt";
    QFile rtf(fname);

    int fpsnum = 0;     //匹配到的帧数
    int endflag = 0;

    rcs::myTracker track(ttype, ftype, smethod);
    Eigen::Matrix3d r;
    Eigen::Vector3d t;
    video.read(frame);  track.Track(frame, K, distCoeffs, H, r, t);
    video.read(frame);  track.Track(frame, K, distCoeffs, H, r, t);

    int fx, fy, cx, cy;
    fx = K(0, 0);       fy = K(1, 1);
    cx = K(0, 2);       cy = K(1, 2);

    pangolin::CreateWindowAndBind("Pangolin_Track", 1024, 768);     //pangolin初始化
    glEnable(GL_DEPTH_TEST);
    pangolin::OpenGlRenderState s_cam(
        pangolin::ProjectionMatrix(1024, 768, fx, fy, cx, cy, 0.2, 2000),
        pangolin::ModelViewLookAt(t[0], t[1], t[2], 0, 0, 0, pangolin::AxisY)
    );
    pangolin::Handler3D handler(s_cam);
    pangolin::View& d_cam = pangolin::CreateDisplay()
        .SetBounds(0.0, 1.0, 0.0, 1.0, -640.0f / 480.0f)
        .SetHandler(&handler);
    pangolin::CreatePanel("menu").SetBounds(0.0, 0.3, 0.0, 0.31);
    pangolin::Var<bool> menu("menu.Trajectory", true, true);
    pangolin::Var<bool> menu2("menu.KeyFrame", true, true);
    string routput1, routput2, routput3, toutput;
    pangolin::Var<string> pangolin_rm1("menu.R", routput1);
    pangolin::Var<string> pangolin_rm2("menu. ", routput2);
    pangolin::Var<string> pangolin_rm3("menu.  ", routput3);
    pangolin::Var<string> pangolin_vt("menu.t", toutput);

    WId wid = (WId)FindWindow(L"pangolin", L"Pangolin_Track");
    qDebug()<<"wid:"<<wid;
    QWindow *mw = QWindow::fromWinId(wid);
    QWidget *m_widget = QWidget::createWindowContainer(mw, this, Qt::Widget);
//    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->addWidget(m_widget);

//    rcs::myTracker track(ttype, ftype, smethod);
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
            if (ok && track.frameNum != 1){
                  /*pangolin可视化*/
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
                        glColor3f(0.f, 1.f, 1.f);
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
                ui->s3log->append("||t||="+QString::number(tVec.norm()));
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
                rtname = "./data/"+Getfname(videopath)+ft+sm+"-"+QString::number(fpsnum)+".txt";
                QFile::rename(fname, rtname);
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            d_cam.Activate(s_cam);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

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
                    glColor3f(0.f, 1.f, 1.f);
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
//    video.release();
//    ui->s3log->append("一共匹配到"+QString::number(fpsnum)+"帧！");
//    rtname = "./data/"+Getfname(videopath)+ft+sm+"-"+QString::number(fpsnum)+".txt";
//    QFile::rename(fname, rtname);
}


void MainWindow::on_track_clicked()     //step3追踪
{
    rcs::trackerType ttype;
    rcs::featureType ftype;
    rcs::solveMethod smethod;

    if(ui->orb->isChecked())
        {ftype = rcs::ORB; ft = "-orb";}
    else if(ui->sift->isChecked())
        ftype = rcs::SIFT;
    else
        {ftype = rcs::SURF; ft = "-surf";}

    if(ui->kcf->isChecked())
        ttype = rcs::KCF;
    else if(ui->boosting->isChecked())
        ttype = rcs::BOOSTING;
    else if(ui->csrt->isChecked())
        ttype = rcs::CSRT;
    else if(ui->mil->isChecked())
        ttype = rcs::MIL;
    else if(ui->tld->isChecked())
        ttype = rcs::TLD;
    else
        ttype = rcs::MEDIANFLOW;

    if(ui->pnp->isChecked())
        {smethod = rcs::PnP; sm = "-pnp";}
    else
        {smethod = rcs::Zhang; sm = "-zhang";}

    if(ui->quat->isChecked())
        tname = "q";
    else if(ui->rmat->isChecked())
        tname = "r";

/*若只进行第三步，使用一下语句来给出H文件*/
    if(hname.isEmpty())
        hname = "./data/H.txt";

    Eigen::Matrix3d H = GetMatrix(hname);
    Eigen::Matrix3d K = GetMatrix("./data/K.txt");
    cv::Mat distCoeffs = GetMat("./data/distCoeffs.txt", 1, 5);

    track(H, K, distCoeffs, ttype, ftype, smethod);
    flagtrack = true;
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


void MainWindow::on_three2two_clicked()     //step3到step2
{
    ui->tabWidget->setCurrentIndex(1);
}


void MainWindow::on_three2four_clicked()     //step3到step4
{
    if(!flagtrack)
        QMessageBox::warning(NULL, QString("提示"),
                             QString("请先开始跟踪"),
                             QString("确定"));
    else{
        ui->tabWidget->setCurrentIndex(3);
    }
}


/////////////////////////////////////////////////////////////step4/////////////////////////////////////////////////////////////
void MainWindow::on_four2three_clicked()     //step4到step3
{
    ui->tabWidget->setCurrentIndex(2);
}


void MainWindow::on_finish_clicked()     //完成可视化，关闭窗口
{
    QApplication::exit();
}

