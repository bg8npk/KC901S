#ifndef KCSCALEWIDGET_H
#define KCSCALEWIDGET_H

#include "qwt_scale_widget.h"
#include "qwt_scale_draw.h"

class KCScaleWidget : public QwtScaleWidget
{
    Q_OBJECT
public:
    explicit KCScaleWidget(QWidget *parent = 0);
    KCScaleWidget(QwtScaleDraw::Alignment, QWidget *parent = 0);

signals:
    void scaleChanged();

protected:
    void scaleChange();

public slots:

};

#endif // KCSCALEWIDGET_H
