#include "kcscalewidget.h"


KCScaleWidget::KCScaleWidget(QWidget *parent) :
    QwtScaleWidget(parent)
{
}

KCScaleWidget::KCScaleWidget(QwtScaleDraw::Alignment align, QWidget *parent) :
    QwtScaleWidget(align, parent)
{

}

void KCScaleWidget::scaleChange()
{
    emit scaleChanged();
    QwtScaleWidget::scaleChange();
}
