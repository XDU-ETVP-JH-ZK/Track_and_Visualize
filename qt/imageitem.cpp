#include <imageitem.h>

ImageItem::ImageItem(QGraphicsItem *parent): QGraphicsPixmapItem(parent)
{
}

ImageItem::ImageItem(const QPixmap& pixmap) : QGraphicsPixmapItem(pixmap)
{
    m_pix = pixmap;
    m_scaleDafault = 0;
    m_scaleValue = 0;
}

ImageItem::~ImageItem()
{
}

void ImageItem::setGraphicsViewWH(int nwidth, int nheight)
{
    int nImgW = m_pix.width();
    int nImgH = m_pix.height();
//    qDebug()<<nImgW<<" "<<nwidth;
    qreal temp1 = nwidth*1.0/nImgW;
    qreal temp2 = nheight*1.0/nImgH;
    m_scaleDafault = (temp1 > temp2) ? temp2 : temp1;
//    if(temp1 > temp2)
//        m_scaleDafault = temp2;
//    else
//        m_scaleDafault = temp1;
    setScale(m_scaleDafault);
    m_scaleValue = m_scaleDafault;
}

QRectF ImageItem::boundingRect() const
{
    return QRectF(0, 0, m_pix.width(), m_pix.height());
}

void ImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                      QWidget *)
{
    painter->drawPixmap(0, 0, m_pix);
}

void ImageItem::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if(event->delta() > 0 && m_scaleValue >= 50)
        return;
    else if(event->delta() < 0 && m_scaleValue <= m_scaleDafault)
        m_scaleValue = m_scaleDafault;
    else{
        qreal qrealOriginScale = m_scaleValue;
        if(event->delta() > 0)//鼠标滚轮向前滚动
            m_scaleValue*=1.1;//每次放大10%
        else
            m_scaleValue*=0.9;//每次缩小10%
        setScale(m_scaleValue);
        if(event->delta() > 0)
            moveBy(-event->pos().x()*qrealOriginScale*0.1, -event->pos().y()*qrealOriginScale*0.1);
        else
            moveBy(event->pos().x()*qrealOriginScale*0.1, event->pos().y()*qrealOriginScale*0.1);
    }
}

qreal ImageItem::getScaleValue() const
{
    return m_scaleValue;
}

//int n = 0;
//double m[3][2];

//void ImageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
//{


//    int x = event->scenePos().x();
//    int y = event->scenePos().y();
//    qDebug() << "(" << x << ", " << y << ")";
//    m[n][0] = x;
//    m[n][1] = y;
////    qDebug()<<m[n][0]<<" "<<m[n][1];

//    n++;
//    if(n == 3){
//        n = 0;
//        double x2, y2;
//        x2 = (m[0][0] + m[1][0] + m[2][0]) / 3;
//        y2 = (m[0][1] + m[1][1] + m[2][1]) / 3;
//        qDebug()<<x2<<" "<<y2;
////        QString coordinate = QString::number(x2)+" "+QString::number(y2);
//        QString coordinate = QString::number(x2, 'f', 3) + " " + QString::number(y2, 'f', 3);
//        QDir dir;
//        if(!dir.exists("data"))
//            dir.mkdir("data");
//        QFile f("./data/coordinate.txt");
//        f.open(QIODevice::WriteOnly);
//        QTextStream txtOutput(&f);
//    //    txtOutput << x << " " << y << endl;
//        txtOutput << coordinate << "\n";
//        f.close();
//    }

////    QString coordinate = QString::number(x2)+" "+QString::number(y2);
////    QDir dir;
////    if(!dir.exists("data"))
////        dir.mkdir("data");
////    QFile f("./data/coordinate.txt");
////    f.open(QIODevice::WriteOnly);
////    QTextStream txtOutput(&f);
//////    txtOutput << x << " " << y << endl;
////    txtOutput << coordinate << "\n";
////    f.close();

//}
