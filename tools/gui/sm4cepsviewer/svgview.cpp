#include "svgview.h"

#include <QSvgRenderer>

#include <QWheelEvent>
#include <QMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QPaintEvent>
#include <qmath.h>

#ifndef QT_NO_OPENGL
#include <QGLWidget>
#endif

SvgView::SvgView(QWidget *parent)
    : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);
    setViewportUpdateMode(FullViewportUpdate);

}

bool SvgView::reload(QString svg ){
    QGraphicsScene *s = scene();
    QScopedPointer<QGraphicsSvgItem> svgItem(new QGraphicsSvgItem("out.svg"));
    if (!svgItem->renderer()->isValid())
       return false;
     s->clear();
     resetTransform();
     m_svgItem = svgItem.take();
     m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
     m_svgItem->setCacheMode(QGraphicsItem::NoCache);
     m_svgItem->setZValue(0);
     s->addItem(m_svgItem);
     return true;
}

void SvgView::reload(){
    reload("");
}
