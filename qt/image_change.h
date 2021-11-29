#ifndef IMAGE_CHANGE_H
#define IMAGE_CHANGE_H

#include <QMainWindow>

class Images_Change
{
public:
    Images_Change();
    QString NextImage(QStringList list);
    QString PreImage(QStringList list);
    int index;
};


#endif // IMAGE_CHANGE_H
