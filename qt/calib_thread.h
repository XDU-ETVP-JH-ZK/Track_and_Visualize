#ifndef CALIB_THREAD_H
#define CALIB_THREAD_H

#include <QThread>
#include <QMessageBox>

#include "Launch.h"


class CalibThread : public QThread
{
    Q_OBJECT
public:
    CalibThread();

protected:
    void run();

signals:
    void completed();

};

#endif // CALIB_THREAD_H
