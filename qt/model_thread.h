#ifndef MODEL_THREAD_H
#define MODEL_THREAD_H

#include <QThread>
#include <QMessageBox>

class ModelThread : public QThread
{
    Q_OBJECT
public:
    ModelThread();

protected:
    void run();

signals:
    void succeed();

};

#endif // MODEL_THREAD_H
