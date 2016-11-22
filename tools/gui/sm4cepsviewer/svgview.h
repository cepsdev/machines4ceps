#ifndef SVGVIEW_H
#define SVGVIEW_H


#include <QGraphicsView>
#include <QString>

class QGraphicsSvgItem;
class QSvgRenderer;
class QWheelEvent;
class QPaintEvent;


class SvgView : public QGraphicsView{
    Q_OBJECT
public:
    explicit SvgView(QWidget *parent = nullptr);
    bool load_from_file(QString svg);
private:
    QGraphicsSvgItem *m_svgItem = nullptr;
};

#endif // SVGVIEW_H
