#include <imageitem.h>

ImageItem::ImageItem(QGraphicsItem *parent): QGraphicsPixmapItem(parent)
{
}

ImageItem::ImageItem(const QPixmap& pixmap) : QGraphicsPixmapItem(pixmap)
{
}

ImageItem::~ImageItem()
{
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
