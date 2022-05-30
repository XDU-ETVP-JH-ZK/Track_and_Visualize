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
    QGraphicsScene  *qgraphicsScene = new QGraphicsScene;//Ҫ��QGraphicsView�ͱ���Ҫ��QGraphicsScene��������
    m_Image = new ImageWidget(&ConvertPixmap);//ʵ������ImageWidget�Ķ���m_Image������̳���QGraphicsItem�����Լ�д����
    int nwith = view->width();//��ȡ����ؼ�Graphics View�Ŀ��
    int nheight = view->height();//��ȡ����ؼ�Graphics View�ĸ߶�
    m_Image->setQGraphicsViewWH(nwith,nheight);//������ؼ�Graphics View��width��height������m_Image��
    qgraphicsScene->addItem(m_Image);//��QGraphicsItem�����Ž�QGraphicsScene��
    /*ʹ�Ӵ��Ĵ�С�̶���ԭʼ��С��������ͼƬ�ķŴ���Ŵ�Ĭ��״̬��ͼƬ�Ŵ��ʱ���Ӵ����߻��Զ����ֹ������������Ӵ��ڵ���Ұ���󣩣�
        ��ֹͼƬ�Ŵ��������С��ʱ���Ӵ�̫���������۲�ͼƬ*/
    view->setSceneRect(QRectF(-(nwith/2),-(nheight/2),nwith,nheight));
    view->setScene(qgraphicsScene);
    view->setFocus();//������Ľ������õ���ǰGraphics View�ؼ�
}
