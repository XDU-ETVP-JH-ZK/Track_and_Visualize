#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <Windows.h>

class CameraThread : public QThread
{
    Q_OBJECT
public:
    CameraThread();
protected:
    void run();
signals:
    void hide();
};

#endif // CAMERATHREAD_H
