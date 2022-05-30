#ifndef PANGOLINTHREAD_H
#define PANGOLINTHREAD_H

#include <QThread>
#include <pangolin/pangolin.h>
#include <opencv2/highgui.hpp>
#include <opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <Windows.h>
#include <vector>
#include <QFile>
#include <QDateTime>

#include "reconstruction.h"

class PangolinThread : public QThread
{
    Q_OBJECT
public:
    PangolinThread();
protected:
    void run();
signals:
    void sendwid();
    void sendata(QString);
    void sendimg1();
    void sendimg2();
    void sendlog();
    void savelog();
    void logend(QString);
};

#endif // PANGOLINTHREAD_H
