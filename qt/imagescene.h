#ifndef IMAGESCENE_H
#define IMAGESCENE_H
#include <QGraphicsScene>


class ImageScene : public QGraphicsScene
{
public:
    ImageScene();
    ~ImageScene();
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
};

#endif // IMAGESCENE_H
