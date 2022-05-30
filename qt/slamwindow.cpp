#include "slamwindow.h"
#include "ui_slamwindow.h"

QList<QCameraDevice> cameras2;
int cameraid;
QString vpath;

SlamWindow::SlamWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SlamWindow)
{
    ui->setupUi(this);

    ui->cameras->addItem("chose video");
    cameras2 = QMediaDevices::videoInputs();
    for(auto &i : cameras2)
        ui->cameras->addItem(i.description());
}

SlamWindow::~SlamWindow()
{
    delete ui;
}


void SlamWindow::on_pushButton_clicked()
{

}


void SlamWindow::on_pushButton_2_clicked()
{

}


void SlamWindow::on_cameras_activated(int index)
{
    cameraid = index;
    if(cameraid == 0){
        vpath = QFileDialog::getOpenFileName(this, "", "/", "ÊÓÆµÎÄ¼þ(*.mp4 *.avi);;");
        if(vpath.isEmpty())
            return;
    }
}

