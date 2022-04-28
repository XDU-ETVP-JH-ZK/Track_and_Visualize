#include "image_change.h"
#include <QMainWindow>
#include <QStringList>
#include <qdir.h>
/*构造函数*/
Images_Change::Images_Change()
{
    index = 0;
}

/*切换图片的方法*/
QString Images_Change::NextImage(QStringList list){
    int n = list.size();
    index++;
    if(index>=n)    //图片显示完后，就把index归0，重新开始显示第一张图片
        index=0;
    return(list[index]);  //返回图片的路径
}
QString Images_Change::PreImage(QStringList list){
    int n = list.size();
    index--;
    if(index<=0)    //图片显示完后，就把index归n-1，显示最后一张图片
        index=n-1;
    return(list[index]);  //返回图片的路径
}
