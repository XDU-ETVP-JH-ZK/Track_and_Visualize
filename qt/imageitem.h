#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QWidget>
#include <QtGui>
#include <QPixmap>
#include <QPainter>
#include <QRectF>
#include <QMouseEvent>
#include <QPointF>
#include <QDragEnterEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsItem>

enum Enum_ZoomState2{
    NO_STATE2,
    RESET2,
    ZOOM_IN2,
    ZOOM_OUT2
};

class ImageItem : public QGraphicsPixmapItem
{
public:
    ImageItem(QGraphicsItem *parent);
    ImageItem(const QPixmap& pixmap);
    ~ImageItem();
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void wheelEvent(QGraphicsSceneWheelEvent *event);
    void setGraphicsViewWH(int nwidth, int nheight);
    qreal getScaleValue() const;
private:
    qreal m_scaleValue;
    qreal m_scaleDafault;
    QPixmap m_pix;
};


#endif // IMAGEITEM_H
