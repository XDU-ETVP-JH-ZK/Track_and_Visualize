#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>

class ImageItem : public QGraphicsPixmapItem
{
public:
    ImageItem(QGraphicsItem *parent);
    ImageItem(const QPixmap& pixmap);
    ~ImageItem();
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
};


#endif // IMAGEITEM_H
