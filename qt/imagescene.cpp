#include <imagescene.h>


ImageScene::ImageScene(): QGraphicsScene()
{
}


ImageScene::~ImageScene()
{
}

void ImageScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsScene::mousePressEvent(event); // 将点击事件向下传递到item中
}


