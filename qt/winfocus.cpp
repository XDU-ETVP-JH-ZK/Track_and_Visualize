#include "winfocus.h"
#include "ui_winfocus.h"

QString newpath;

WinFocus::WinFocus(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WinFocus)
{
    ui->setupUi(this);

    ui->tabWidget->setCurrentIndex(0);
}

WinFocus::~WinFocus()
{
    delete ui;
}

void WinFocus::on_pushButton_clicked()
{
    QString pinit = newpath.isEmpty() ? "/" : newpath;
    QString path = QFileDialog::getOpenFileName(this, "picture", pinit, "picture file(*.jpg *.png);;");
    if(path.isEmpty())
        return;
    QFileInfo info(path);
    newpath = info.absolutePath();

    QFileSystemModel *fsm = new QFileSystemModel();
    fsm->setRootPath(newpath);
    ui->treeView->setModel(fsm);
    ui->treeView->setRootIndex(fsm->index(newpath));
    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setColumnHidden(2, true);
    ui->treeView->setColumnHidden(3, true);

    QStringList tname;
    tname.push_back(newpath);
    QStandardItemModel *sim = new QStandardItemModel();
    sim->setHorizontalHeaderLabels(tname);
    ui->treeView->header()->setModel(sim);
    ui->treeView->resizeColumnToContents(0);

    QImage img(path);
    recvShowPicSignal(img, ui->graphicsView);
}


void WinFocus::on_pushButton_4_clicked()
{
    QString pinit = newpath.isEmpty() ? "/" : newpath;
    QStringList path = QFileDialog::getOpenFileNames(this, "picture", pinit, "picture file(*.jpg *.png);;");
    if(path.isEmpty())
        return;
    QFileInfo info(path[0]);
    newpath = info.absolutePath();

    QFileSystemModel *fsm = new QFileSystemModel();
    fsm->setRootPath(newpath);
    ui->treeView_2->setModel(fsm);
    ui->treeView_2->setRootIndex(fsm->index(newpath));
    ui->treeView_2->setColumnHidden(1, true);
    ui->treeView_2->setColumnHidden(2, true);
    ui->treeView_2->setColumnHidden(3, true);

    QStringList tname;
    tname.push_back(newpath);
    QStandardItemModel *sim = new QStandardItemModel();
    sim->setHorizontalHeaderLabels(tname);
    ui->treeView_2->header()->setModel(sim);
    ui->treeView_2->resizeColumnToContents(0);

    QImage img1(path[0]), img2;
    recvShowPicSignal(img1, ui->graphicsView_2);
    if(path.size() == 2){
        img2.load(path[1]);
        recvShowPicSignal(img2, ui->graphicsView_4);
    }
}


void WinFocus::recvShowPicSignal(QImage image, QGraphicsView *view)
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
