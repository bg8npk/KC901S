/***************************************************************************
 *   Copyright (C) 2008 by Lisandro Damián Nicanor Pérez Meyer             *
 *   perezmeyer@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include "smithchart.h"
#include <QDebug>
#include <QPainterPath>
#include <QPainter>
#include <QRectF>
#include <QVector>
#include <QPointF>
#include <math.h>

static const double PI = 3.141592654;
static const float angleOfTransmissionR[] = {912.0, 911.5, 911.0, 910.5, 910.0,
                                             909.0, 907.5, 905.5, 903.5, 901.5,
                                             899.0, 896.0, 892.5, 889.5, 885.5,
                                             882.5, 878.0, 874.0, 869.0, 863.5,
                                             858.5, 853.5, 848.0, 842.0, 835.5,
                                             829.5, 823.0, 816.0, 809.0, 802.0,
                                             794.0, 786.5, 779.0, 770.5, 762.0,
                                             754.0, 745.0, 735.5, 726.0, 716.5,
                                             707.5, 697.0, 687.0, 677.5, 666.5,
                                             655.0, 645.5, 634.0, 623.0, 612.0,
                                             600.0, 589.0, 578.0, 566.0, 554.0,
                                             542.0, 529.5, 517.0, 504.5, 492.0,
                                             479.0, 466.0, 454.0, 440.5, 427.5,
                                             414.5, 402.0, 389.0, 375.0, 362.0,
                                             349.0, 335.0, 322.0, 310.0, 297.0,
                                             284.0, 271.0, 258.0, 245.0, 232.0,
                                             223.0, 210.0, 197.0, 188.0, 177.0,
                                             166.0, 155.0, 146.5, 137.0, 130.0,
                                             122.0};

SmithChart::SmithChart(QWidget * parent) : QWidget(parent)
{
    arcsCalculated = false;
    showInterpolatedLine = false;

    // We define pens
    backgroundBrush = QBrush(Qt::white);
    scaleColor = QColor(Qt::black);
    pointDataPen = QPen(QColor("red"), 4.0, Qt::SolidLine, Qt::RoundCap);
    lineDataPen = QPen(QColor("blue"), 1.0);

    thickPen = QPen(scaleColor, 0.75);
    thinPen = QPen(scaleColor, 0.25);
    textPen = QPen(scaleColor, 0.25);
}

SmithChart::~SmithChart()
{
    // Empty the data vector
    dataVector.clear();
}

void SmithChart::clear()
{
    dataVector.clear();
    update();
}

void SmithChart::draw(QPainter * painter)
{

    // We set the pen and brush for the outern circles
    painter->setPen(thickPen);
    painter->setBrush(palette().foreground());

    // The outer circle
    QRectF rectangle(-512, -512, 1024, 1024);
    painter->drawArc(rectangle, 0, 5760);

    // Inner thin circle, wavelengths toward generator
    rectangle.adjust(16, 16, -16, -16);
    painter->setPen(thinPen);
    painter->drawArc(rectangle, 0, 5760);

    // Inner thick circle, wavelengths toward load
    rectangle.adjust(16, 16, -16, -16);
    painter->setPen(thickPen);
    painter->drawArc(rectangle, 0, 5760);

    // Inner thin circle, angle of reflection coefficient in degrees
    rectangle.adjust(16, 16, -16, -16);
    painter->setPen(thinPen);
    painter->drawArc(rectangle, 0, 5760);

    // Inner thick circle, angle of transmission coefficient in degrees
    rectangle.adjust(16, 16, -16, -16);
    painter->setPen(thickPen);
    painter->drawArc(rectangle, 0, 5760);


    /*
        The ticks in the wavelengths scales
        The scales advance in 0.002 degrees.
        0.5 [degrees] / 0.002 [degrees/tick] = 250 [tick]
        360º / 250 [tick] = 1.44º/[tick]
        So, i has to advance while i < 250
        Thick marcs: 0.002 * 5 = 0.01 (each time i % 5 == 0)
    */
    for(int i=0; i< 250; i++)
    {
        // We keep the transformation matrix
        painter->save();
        painter->rotate(-i*1.44);

        if(i % 5 == 0)
        {
            painter->setPen(thickPen);
            painter->drawLine(493, 0, 499, 0);
        }
        else
        {
            painter->setPen(thinPen);
            painter->drawLine(493, 0, 499, 0);
        }
        painter->restore();
    }

    /*
        Ticks in the angle of reflection coefficient scale
        This scale goes (-180º,180º], in steps of two degrees.
        So for a half circle we have:
        180º/2[º/tick] = 90 [tick]
    */
    for(int i=90; i > -90; i--)
    {
        // We keep the transformation matrix
        painter->save();
        painter->rotate(-i*2.0);

        if(i % 5 == 0)
        {
            painter->setPen(thickPen);
            painter->drawLine(464, 0, 468, 0);
        }
        else
        {
            painter->setPen(thinPen);
            painter->drawLine(464, 0, 468, 0);
        }
        painter->restore();
    }

    /*
        The numbers in the wavelengths scales
        This time there are only:
        0.5 [degrees] / 0.01 [degrees/tick] = 50 [tick]
        360º / 50 [tick] = 7.2 [º/tick]
    */

    // We set the font for the scale
    painter->setFont(QFont("monospace", 8, QFont::Light));
    painter->setPen(textPen);

    for(int i=0; i< 50; i++)
    {
        // We keep the transformation matrix
        painter->save();
        painter->rotate(-90);
        painter->rotate((-i*7.2));

        // Wavelengths towards generator scale
        if(i == 0)
        {
            painter->drawText(-14,-595,100,100,
                              Qt::AlignLeft | Qt::AlignBottom,
                              QString::number(0.0,'f',2));
        }
        else
        {
            painter->drawText(-14,-595,100,100,
                              Qt::AlignLeft | Qt::AlignBottom,
                              QString::number(0.5-(i*0.01),'f',2));
        }
        // Wavelengths towards load scale
        painter->drawText(-14,-578,100,100,
                          Qt::AlignLeft | Qt::AlignBottom,
                          QString::number(i*0.01,'f',2));

        painter->restore();
    }

    /*
        The numbers in the angle of reflection coefficient scale
        This scale goes (-180º,180º], in steps of 10 degrees.
        So for a half circle we have:
        180º/10[º/tick] = 18 [tick]
    */
    for(int i=18; i > -18; i--)
    {
        // We keep the transformation matrix
        painter->save();
        painter->rotate(90);
        painter->rotate((-i*10.0));

        if(i >= 0)
        {
            // From 180º to 0º
            // The formula in X centers the text in the ticks
            painter->drawText((int)((-4.0/9.0)*i-4),-565,100,100,
                              Qt::AlignLeft | Qt::AlignBottom,
                              QString::number(i*10));
        }
        else
        {
            // From 0º to -180º
            // The formula in X centers the text in the ticks
            painter->drawText((int)((4.0/9.0)*i-10),-565,100,100,
                              Qt::AlignLeft | Qt::AlignBottom,
                              QString::number(i*10));
        }
        painter->restore();
    }

    /*
        Ticks in the angle of transmission coefficient scale
        This scale goes [0,90], in steps of 1.
        This scale is the degrees that has the line that goes across the point
        Z = 0 + j0 and the intersection of the scale between the X axis.
    */
    QPointF first;
    QPointF second;
    painter->setPen(thinPen);

    for(int i=0; i < 91; i++)
    {
        second.rx() = angleOfTransmissionR[i]*cos(i*PI/180.0);
        // 448.0 is the distance from the center to Z = 0 + j0
        second.rx() -= 448.0;
        second.ry() = -angleOfTransmissionR[i]*sin(i*PI/180.0);

        double R = sqrt((second.x()*second.x() + 448.0) + second.y()*second.y());

        first.rx() = second.x() - (second.x() * 7.0 / R);
        first.ry() = second.y() - (second.y() * 7.0 / R);

        // Draw the upper ticks
        painter->drawLine(first, second);

        // Draw the lower ticks
        first.ry() *= -1;
        second.ry() *= -1;
        painter->drawLine(first, second);

    }


    // The resistance or conductance component
    painter->setPen(thickPen);
    painter->drawLine(448, 0, -448, 0);

    // Zero impedance scale
    painter->save();
    painter->setPen(textPen);
    painter->rotate(-90);
    painter->drawText(QPointF(0.0,-438.0),
                      QString::number(0.0,'f',1));
    painter->restore();

    // Finally we draw
    painter->strokePath(thickArcsPath, thickPen);
    painter->strokePath(thinArcsPath, thinPen);

    // Draw the data
    painter->setPen(pointDataPen);
    for(int i=0; i < dataVector.size(); i++)
        painter->drawPoint(dataVector.at(i));

    if(showInterpolatedLine)
    {
        painter->setPen(lineDataPen);
        for(int i=1; i < dataVector.size(); i++)
            painter->drawLine(dataVector.at(i-1), dataVector.at(i));
    }
}

void SmithChart::setZ(const double & real, const double & imaginary)
{
    // Calculate the point coordinates
    QPointF point = calculateZCoordinates(real, imaginary);
    dataVector.append(point);

    update();
}

void SmithChart::setZ(const QVector<QPointF> ZVector)
{
    for(int i=0; i < ZVector.size(); i++)
    {
        QPointF point = calculateZCoordinates(ZVector.at(i).x(),
                                              ZVector.at(i).y());
        dataVector.append(point);
    }

    update();
}

void SmithChart::setReflection(const QVector<QPointF> L)
{
    for(int i=0; i < L.size(); i++)
    {
        QPointF point = calculateXY(L.at(i).x(), L.at(i).y());
        dataVector.append(point);
    }

    update();
}

void SmithChart::setStyle(QBrush background, QColor scale, QPen datapoint, QPen dataline)
{
    backgroundBrush = background;
    scaleColor = scale;
    thickPen = QPen(scaleColor, 0.75);
    thinPen = QPen(scaleColor, 0.25);
    textPen = QPen(scaleColor, 0.25);
    pointDataPen = datapoint;
    lineDataPen = dataline;
}

qreal SmithChart::padding()
{
    return m_padding;
}

void SmithChart::setPadding(qreal padding)
{
    m_padding = padding;
    emit paddingChanged();
}

void SmithChart::showLine(bool show)
{
    showInterpolatedLine = show;
    update();
}

void SmithChart::paintEvent(QPaintEvent * /* the event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBackground(backgroundBrush);
    painter.fillRect(0, 0, width(), height(), backgroundBrush);

    int side = qMin(width(), height()) * (1.0 - m_padding);

    painter.setViewport((width()-side)/2, (height()-side)/2, side, side);
    painter.setWindow(-512, -512, 1024, 1024);

    if(!arcsCalculated)
    {
        calculateInsideArcs();
        arcsCalculated = true;
    }

    draw(&painter);
}

void SmithChart::calculateInsideArcs()
{
    /*
    Inside arcs - Formulas derived from:
    http://en.wikipedia.org/wiki/Smith_chart#Circles_of_Constant_Normalised_Resistance_and_Constant_Normalised_Reactance
    */

    // Z = 0.01
    drawConstantZArc(thinPen, 0.01, 157.5, 22.5);
    // Z = 0.02
    drawConstantZArc(thinPen, 0.02, 128.0, 52.0);
    // Z = 0.03
    drawConstantZArc(thinPen, 0.03, 158.0, 22.0);
    // Z = 0.04
    drawConstantZArc(thinPen, 0.04, 128.5, 51.5);
    // Z = 0.05
    drawConstantZArc(thickPen, 0.05, 158.5, 21.5);
    drawConstantZArc(thinPen, 0.05, 93.0, 36.0);
    // Z = 0.06
    drawConstantZArc(thinPen, 0.06, 129.5, 50.5);
    // Z = 0.07
    drawConstantZArc(thinPen, 0.07, 159.0, 21.0);
    // Z = 0.08
    drawConstantZArc(thinPen, 0.08, 130.5, 49.5);
    // Z = 0.09
    drawConstantZArc(thinPen, 0.09, 159.5, 20.5);
    // Z = 0.1
    drawConstantZArc(thickPen, 0.1, 95.5, 84.5);
    drawConstantZArc(thinPen, 0.1, 57.5, 38.0);

    // Z = 0.11
    drawConstantZArc(thinPen, 0.11, 159.5, 20.5);
    // Z = 0.12
    drawConstantZArc(thinPen, 0.12, 132.0, 48.0);
    // Z = 0.13
    drawConstantZArc(thinPen, 0.13, 160.0, 20.0);
    // Z = 0.14
    drawConstantZArc(thinPen, 0.14, 132.5, 47.5);
    // Z = 0.15
    drawConstantZArc(thickPen, 0.15, 160.25, 19.75);
    drawConstantZArc(thinPen, 0.15, 98.0, 35.0);
    // Z = 0.16
    drawConstantZArc(thinPen, 0.16, 133.5, 46.5);
    // Z = 0.17
    drawConstantZArc(thinPen, 0.17, 160.5, 19.5);
    // Z = 0.18
    drawConstantZArc(thinPen, 0.18, 134.0, 46.0);
    // Z = 0.19
    drawConstantZArc(thinPen, 0.19, 161.0, 19.0);
    // Z = 0.2
    drawConstantZArc(thickPen, 0.2, 62.0, 118.0);
    drawConstantZArc(thinPen, 0.2, 27.0, 35.0);

    // Z = 0.22
    drawConstantZArc(thinPen, 0.22, 135.5, 44.5);
    // Z = 0.24
    drawConstantZArc(thinPen, 0.24, 136.0, 44.0);
    // Z = 0.25
    drawConstantZArc(thinPen, 0.25, 103.0, 33.5);
    // Z = 0.26
    drawConstantZArc(thinPen, 0.26, 136.75, 43.25);
    // Z = 0.28
    drawConstantZArc(thinPen, 0.28, 137.5, 42.5);
    // Z = 0.3
    drawConstantZArc(thickPen, 0.3, 105.0, 75.0);
    drawConstantZArc(thinPen, 0.3, 66.0, 39.0);
    // Z = 0.32
    drawConstantZArc(thinPen, 0.32, 138.75, 41.25);
    // Z = 0.34
    drawConstantZArc(thinPen, 0.34, 139.25, 40.75);
    // Z = 0.35
    drawConstantZArc(thinPen, 0.35, 107.0, 32.5);
    // Z = 0.36
    drawConstantZArc(thinPen, 0.36, 140.0, 40.0);
    // Z = 0.38
    drawConstantZArc(thinPen, 0.38, 140.5, 39.5);
    // Z = 0.4
    drawConstantZArc(thickPen, 0.4, 70.0, 110.0);
    drawConstantZArc(thinPen, 0.4, 31.0, 39.0);
    // Z = 0.42
    drawConstantZArc(thinPen, 0.42, 141.5, 38.5);
    // Z = 0.44
    drawConstantZArc(thinPen, 0.44, 142.0, 38.0);
    // Z = 0.45
    drawConstantZArc(thinPen, 0.45, 111.0, 31.5);
    // Z = 0.46
    drawConstantZArc(thinPen, 0.46, 142.5, 37.5);
    // Z = 0.48
    drawConstantZArc(thinPen, 0.48, 143.0, 37.0);
    // Z = 0.5
    drawConstantZArc(thickPen, 0.5, 113.0, 67.0);
    drawConstantZArc(thinPen, 0.5, 74.0, 39.0);

    // Z = 0.55
    drawConstantZArc(thinPen, 0.55, 115.0, 65.0);
    // Z = 0.6
    drawConstantZArc(thickPen, 0.6, 77.5, 102.5);
    drawConstantZArc(thinPen, 0.6, 35.5, 42.0);
    // Z = 0.65
    drawConstantZArc(thinPen, 0.65, 118.0, 62.0);
    // Z = 0.7
    drawConstantZArc(thickPen, 0.7, 119.5, 60.5);
    drawConstantZArc(thinPen, 0.7, 81.0, 38.5);
    // Z = 0.75
    drawConstantZArc(thinPen, 0.75, 121.0, 59.0);
    // Z = 0.8
    drawConstantZArc(thickPen, 0.8, 84.0, 96.0);
    drawConstantZArc(thinPen, 0.8, 39.5, 44.5);
    // Z = 0.85
    drawConstantZArc(thinPen, 0.85, 123.5, 56.5);
    // Z = 0.9
    drawConstantZArc(thickPen, 0.9, 125.0, 55.0);
    drawConstantZArc(thinPen, 0.9, 87.0, 38.0);
    // Z = 0.95
    drawConstantZArc(thinPen, 0.95, 126.0, 54.0);
    // Z = 1
    drawConstantZArc(thickPen, 1.0, 44.0, 136.0);
    drawConstantZArc(thinPen, 1.0, 22.5, 22.0);

    // Z = 1.1
    drawConstantZArc(thinPen, 1.1, 93.0, 87.0);
    // Z = 1.2
    drawConstantZArc(thickPen, 1.2, 95.5, 84.5);
    drawConstantZArc(thinPen, 1.2, 47.5, 50.0);
    // Z = 1.3
    drawConstantZArc(thinPen, 1.3, 98.0, 82.0);
    // Z = 1.4
    drawConstantZArc(thickPen, 1.4, 100.0, 80.0);
    drawConstantZArc(thinPen, 1.4, 51.0, 50.0);
    // Z = 1.5
    drawConstantZArc(thinPen, 1.5, 103.0, 77.0);
    // Z = 1.6
    drawConstantZArc(thickPen, 1.6, 105.0, 75.0);
    drawConstantZArc(thinPen, 1.6, 55.0, 50.0);
    // Z = 1.7
    drawConstantZArc(thinPen, 1.7, 107.0, 73.0);
    // Z = 1.8
    drawConstantZArc(thickPen, 1.8, 109.0, 71.0);
    drawConstantZArc(thinPen, 1.8, 58.5, 50.0);
    // Z = 1.9
    drawConstantZArc(thinPen, 1.9, 110.5, 69.5);
    // Z = 2
    drawConstantZArc(thickPen, 2.0, 62.0, 118.0);
    drawConstantZArc(thinPen, 2.0, 17.0, 46.0);

    // Z = 2.2
    drawConstantZArc(thinPen, 2.2, 65.0, 115.0);
    // Z = 2.4
    drawConstantZArc(thinPen, 2.4, 68.0, 112.0);
    // Z = 2.6
    drawConstantZArc(thinPen, 2.6, 72.0, 108.0);
    // Z = 2.8
    drawConstantZArc(thinPen, 2.8, 75.0, 105.0);
    // Z = 3
    drawConstantZArc(thickPen, 3.0, 78.0, 102.0);
    drawConstantZArc(thinPen, 3.0, 44.0, 34.0);
    // Z = 3.2
    drawConstantZArc(thinPen, 3.2, 79.5, 100.5);
    // Z = 3.4
    drawConstantZArc(thinPen, 3.4, 82.0, 98.0);
    // Z = 3.6
    drawConstantZArc(thinPen, 3.6, 85.0, 95.0);
    // Z = 3.8
    drawConstantZArc(thinPen, 3.8, 88.0, 92.0);
    // Z = 4
    drawConstantZArc(thickPen, 4.0, 90.0, 90.0);
    drawConstantZArc(thinPen, 4.0, 54.0, 36.0);
    // Z = 4.2
    drawConstantZArc(thinPen, 4.2, 93.0, 87.0);
    // Z = 4.4
    drawConstantZArc(thinPen, 4.4, 96.0, 84.0);
    // Z = 4.6
    drawConstantZArc(thinPen, 4.6, 98.0, 82.0);
    // Z = 4.8
    drawConstantZArc(thinPen, 4.8, 99.0, 81.0);
    // Z = 5
    drawConstantZArc(thickPen, 5.0, 62.0, 118.0);

    // Z = 6
    drawConstantZArc(thinPen, 6.0, 70.0, 110.0);
    // Z = 7
    drawConstantZArc(thinPen, 7.0, 79.0, 101.0);
    // Z = 8
    drawConstantZArc(thinPen, 8.0, 84.0, 96.0);
    // Z = 9
    drawConstantZArc(thinPen, 9.0, 89.0, 91.0);
    // Z = 10
    drawConstantZArc(thickPen, 10.0, 20.0, 160.0);

    // Z = 12
    drawConstantZArc(thinPen, 12.0, 20.0, 160.0);
    // Z = 14
    drawConstantZArc(thinPen, 14.0, 20.0, 160.0);
    // Z = 16
    drawConstantZArc(thinPen, 16.0, 20.0, 160.0);
    // Z = 18
    drawConstantZArc(thinPen, 18.0, 20.0, 160.0);
    // Z = 20
    drawConstantZArc(thickPen, 20.0, 20.0, 160.0);

    // Z = 30
    drawConstantZArc(thinPen, 30.0, 20.0, 160.0);
    // Z = 40
    drawConstantZArc(thinPen, 40.0, 20.0, 160.0);
    // Z = 50
    drawConstantZArc(thickPen, 50.0, 0.0, 180.0);

    /* Constant ro arcs */
    // Ro = 0.01
    drawConstantRoArc(thinPen, 0.01, 268.855, 0.19);
    // Ro = 0.02
    drawConstantRoArc(thinPen, 0.02, 267.71, 0.76);
    // Ro = 0.03
    drawConstantRoArc(thinPen, 0.03, 266.57, 0.57);
    // Ro = 0.04
    drawConstantRoArc(thinPen, 0.04, 265.41, 1.526);
    // Ro = 0.05
    drawConstantRoArc(thickPen, 0.05, 264.28, 0.95);
    drawConstantRoArc(thinPen, 0.05, 266.15, 0.98);

    // Ro = 0.06
    drawConstantRoArc(thinPen, 0.06, 263.14, 2.28);
    // Ro = 0.07
    drawConstantRoArc(thinPen, 0.07, 262.0, 1.3);
    // Ro = 0.08
    drawConstantRoArc(thinPen, 0.08, 260.85, 3.05);
    // Ro = 0.09
    drawConstantRoArc(thinPen, 0.09, 259.7, 1.7);
    // Ro = 0.1
    drawConstantRoArc(thickPen, 0.1, 258.6, 5.65);
    drawConstantRoArc(thinPen, 0.1, 264.25, 1.95);

    // Ro = 0.11
    drawConstantRoArc(thinPen, 0.11, 257.45, 2.05);
    // Ro = 0.12
    drawConstantRoArc(thinPen, 0.12, 256.3, 4.55);
    // Ro = 0.13
    drawConstantRoArc(thinPen, 0.13, 255.18, 2.4);
    // Ro = 0.14
    drawConstantRoArc(thinPen, 0.14, 254.05, 5.3);
    // Ro = 0.15
    drawConstantRoArc(thickPen, 0.15, 252.95, 2.8);
    drawConstantRoArc(thinPen, 0.15, 258.6, 2.8);

    // Ro = 0.16
    drawConstantRoArc(thinPen, 0.16, 251.8, 6.0);
    // Ro = 0.17
    drawConstantRoArc(thinPen, 0.17, 250.75, 3.1);
    // Ro = 0.18
    drawConstantRoArc(thinPen, 0.18, 249.6, 6.7);
    // Ro = 0.19
    drawConstantRoArc(thinPen, 0.19, 248.45, 3.5);


    // Ro = 0.2
    drawConstantRoArc(thickPen, 0.2, 247.4, 14.95);
    drawConstantRoArc(thinPen, 0.2, 262.35, 3.8);
    // Ro = 0.22
    drawConstantRoArc(thinPen, 0.22, 245.2, 8.1);
    // Ro = 0.24
    drawConstantRoArc(thinPen, 0.24, 243.0, 8.8);
    // Ro = 0.25
    drawConstantRoArc(thinPen, 0.25, 251.1, 4.7);
    // Ro = 0.26
    drawConstantRoArc(thinPen, 0.26, 240.85, 9.5);
    // Ro = 0.28
    drawConstantRoArc(thinPen, 0.28, 238.75, 10.15);

    // Ro = 0.3
    drawConstantRoArc(thickPen, 0.3, 236.6, 16.3);
    drawConstantRoArc(thinPen, 0.3, 252.9, 5.75);
    // Ro = 0.32
    drawConstantRoArc(thinPen, 0.32, 234.5, 11.4);
    // Ro = 0.34
    drawConstantRoArc(thinPen, 0.34, 232.5, 12.0);
    // Ro = 0.35
    drawConstantRoArc(thinPen, 0.35, 243.75, 6.4);
    // Ro = 0.36
    drawConstantRoArc(thinPen, 0.36, 230.4, 12.6);
    // Ro = 0.38
    drawConstantRoArc(thinPen, 0.38, 228.35, 13.2);

    // Ro = 0.4
    drawConstantRoArc(thickPen, 0.4, 226.4, 28.4);
    drawConstantRoArc(thinPen, 0.4, 254.8, 7.5);
    // Ro = 0.42
    drawConstantRoArc(thinPen, 0.42, 224.4, 14.4);
    // Ro = 0.44
    drawConstantRoArc(thinPen, 0.44, 222.5, 14.8);
    // Ro = 0.45
    drawConstantRoArc(thinPen, 0.45, 236.6, 8.1);
    // Ro = 0.46
    drawConstantRoArc(thinPen, 0.46, 220.6, 15.35);
    // Ro = 0.48
    drawConstantRoArc(thinPen, 0.48, 218.75, 15.75);

    // Ro = 0.5
    drawConstantRoArc(thickPen, 0.5, 216.9, 25.0);
    drawConstantRoArc(thinPen, 0.5, 241.9, 9.0);
    // Ro = 0.55
    drawConstantRoArc(thinPen, 0.55, 212.35, 27.0);
    // Ro = 0.6
    drawConstantRoArc(thickPen, 0.6, 208.1, 39.25);
    drawConstantRoArc(thinPen, 0.6, 247.35, 11.25);
    // Ro = 0.65
    drawConstantRoArc(thinPen, 0.65, 204.0, 30.0);
    // Ro = 0.7
    drawConstantRoArc(thickPen, 0.7, 200.0, 31.4);
    drawConstantRoArc(thinPen, 0.7, 231.4, 12.4);
    // Ro = 0.75
    drawConstantRoArc(thinPen, 0.75, 196.25, 32.75);
    // Ro = 0.8
    drawConstantRoArc(thickPen, 0.8, 192.75, 47.4);
    drawConstantRoArc(thinPen, 0.8, 240.0, 14.75);
    // Ro = 0.85
    drawConstantRoArc(thinPen, 0.85, 189.0, 35.0);
    // Ro = 0.9
    drawConstantRoArc(thickPen, 0.9, 186.0, 35.5);
    drawConstantRoArc(thinPen, 0.9, 221.5, 15.0);
    // Ro = 0.95
    drawConstantRoArc(thinPen, 0.95, 183.0, 36.0);


    // Ro = 1.0
    drawConstantRoArc(thickPen, 1.0, 180.0, 71.0);
    drawConstantRoArc(thinPen, 1.0, 251.0, 8.75);
    // Ro = 1.1
    drawConstantRoArc(thinPen, 1.1, 174.5, 55.25);
    // Ro = 1.2
    drawConstantRoArc(thickPen, 1.2, 169.5, 57.0);
    drawConstantRoArc(thinPen, 1.2, 226.5, 21.0);
    // Ro = 1.3
    drawConstantRoArc(thinPen, 1.3, 165.0, 58.0);
    // Ro = 1.4
    drawConstantRoArc(thickPen, 1.4, 161.0, 59.0);
    drawConstantRoArc(thinPen, 1.4, 220.0, 24.0);
    // Ro = 1.5
    drawConstantRoArc(thinPen, 1.5, 157.0, 60.0);
    // Ro = 1.6
    drawConstantRoArc(thickPen, 1.6, 154.0, 60.0);
    drawConstantRoArc(thinPen, 1.6, 214.0, 26.0);
    // Ro = 1.7
    drawConstantRoArc(thinPen, 1.7, 151.0, 60.0);
    // Ro = 1.8
    drawConstantRoArc(thickPen, 1.8, 148.0, 60.0);
    drawConstantRoArc(thinPen, 1.8, 208.0, 28.5);
    // Ro = 1.9
    drawConstantRoArc(thinPen, 1.9, 145.5, 59.5);

    // Ro = 2
    drawConstantRoArc(thickPen, 2.0, 143.0, 90.0);
    drawConstantRoArc(thinPen, 2.0, 233.0, 26.0);
    // Ro = 2.2
    drawConstantRoArc(thinPen, 2.2, 139.0, 91.0);
    // Ro = 2.4
    drawConstantRoArc(thinPen, 2.4, 135.0, 91.0);
    // Ro = 2.6
    drawConstantRoArc(thinPen, 2.6, 132.0, 91.0);
    // Ro = 2.8
    drawConstantRoArc(thinPen, 2.8, 129.0, 91.0);

    // Ro = 3
    drawConstantRoArc(thickPen, 3.0, 127.0, 90.0);
    drawConstantRoArc(thinPen, 3.0, 217.0, 23.0);
    // Ro = 3.2
    drawConstantRoArc(thinPen, 3.2, 125.0, 89.0);
    // Ro = 3.4
    drawConstantRoArc(thinPen, 3.4, 122.5, 88.0);
    // Ro = 3.6
    drawConstantRoArc(thinPen, 3.6, 120.5, 88.0);
    // Ro = 3.8
    drawConstantRoArc(thinPen, 3.8, 119.0, 87.0);

    // Ro = 4
    drawConstantRoArc(thickPen, 4.0, 118.0, 85.0);
    drawConstantRoArc(thinPen, 4.0, 203.0, 46.0);
    // Ro = 4.2
    drawConstantRoArc(thinPen, 4.2, 116.5, 83.5);
    // Ro = 4.4
    drawConstantRoArc(thinPen, 4.4, 115.0, 83.0);
    // Ro = 4.6
    drawConstantRoArc(thinPen, 4.6, 113.5, 82.5);
    // Ro = 4.8
    drawConstantRoArc(thinPen, 4.8, 113.0, 81.0);

    // Ro = 5
    drawConstantRoArc(thickPen, 5.0, 112.5, 109.0);
    // Ro = 6
    drawConstantRoArc(thinPen, 6.0, 110.5, 129.0);
    // Ro = 7
    drawConstantRoArc(thinPen, 7.0, 109.0, 97.5);
    // Ro = 8
    drawConstantRoArc(thinPen, 8.0, 108.0, 120.5);
    // Ro = 9
    drawConstantRoArc(thinPen, 9.0, 106.0, 89.0);

    // Ro = 10
    drawConstantRoArc(thickPen, 10.0, 104.0, 114.0);
    // Ro = 12
    drawConstantRoArc(thinPen, 12.0, 102.0, 111.0);
    // Ro = 14
    drawConstantRoArc(thinPen, 14.0, 100.0, 106.0);
    // Ro = 16
    drawConstantRoArc(thinPen, 16.0, 98.0, 100.0);
    // Ro = 18
    drawConstantRoArc(thinPen, 18.0, 96.0, 94.0);

    // Ro = 20
    drawConstantRoArc(thickPen, 20.0, 94.0, 88.0);
    // Ro = 50
    drawConstantRoArc(thickPen, 50.0, 92.0, 178.0);
}

QPointF SmithChart::calculateZCoordinates(const double & real,
                                          const double & imaginary)
{
    double x = ((real*real)+(imaginary*imaginary)-1.0);
    x = x / ((real+1.0)*(real+1.0)+(imaginary*imaginary));
    x = x * 448.0;
    /*
    Note that the y coordinate gets inverted, that is because the graphic
    coordinates are inverted.
    */
    double y = 2.0*imaginary/((real+1)*(real+1)+(imaginary*imaginary));
    y = -y * 448.0;

    return QPointF(x, y);
}

QPointF SmithChart::calculateXY(const double & real,
                                const double & imaginary)
{
    double x = real * 448.0;
    /*
    Note that the y coordinate gets inverted, that is because the graphic
    coordinates are inverted.
    */
    double y = -imaginary * 448.0;

    return QPointF(x, y);
}

void SmithChart::drawConstantZArc(QPen pen, double z,
                                  double angle, double span)
{
    double topLeftX = 0;

    // The imaginary part is zero
    topLeftX = ((z*z)-1.0)/((z+1.0)*(z+1.0));
    topLeftX = topLeftX*224.0;

    // Upper arc
    QRectF rectangle = QRectF(2.0*topLeftX,-224.0+topLeftX,
                              448.0-2.0*topLeftX,448.0-2.0*topLeftX);

    rectangle = rectangle.normalized();

    if(pen == thinPen)
    {
        thinArcsPath.arcMoveTo(rectangle, angle);
        thinArcsPath.arcTo(rectangle, angle, span);
    }
    else
    {
        thickArcsPath.arcMoveTo(rectangle, angle);
        thickArcsPath.arcTo(rectangle, angle, span);
    }

    // Lower arcs . We do not need to change the rectangle.
    if(pen == thinPen)
    {
        thinArcsPath.arcMoveTo(rectangle, -angle);
        thinArcsPath.arcTo(rectangle, -angle, -span);
    }
    else
    {
        thickArcsPath.arcMoveTo(rectangle, -angle);
        thickArcsPath.arcTo(rectangle, -angle, -span);
    }

// 	if(drawScale)
// 	{
// 		// Print the scale
// 		painter->save();
// 		painter->setPen(textPen);
// 		painter->rotate(-90);
// 		painter->drawText(QPointF::QPointF(0,2*topLeftX),
// 		                  QString::number(z,'f',precision));
// 		painter->restore();
// 	}
}

void SmithChart::drawConstantRoArc(QPen pen, double ro, double startAngle, double span)
{
    //	Upper arc
    //	The first parameter is the top left X corner of the rectangle
    double topLeftX = (1.0-(1.0/ro))*448.0;
    /*
        The second parameter is the top left Y corner of the rectangle. It is
        calculated taking into account that the biggest inner circle has a radius
        of (1024 / 2) - 64 = 448. As the box has to be as big as the diameter, we
        use 896. Then the circles are scaled by the factor 1/ro.
        Note that the coordinates are the ones normally used in computers, where Y
        is positive downwards, so we must multiply by -1 the value.
    */
    double topLeftY = 896.0*(1.0/ro);
    /*
        The third and fourth parameters are the width and the height.
        As topLeftY, it should have the length of the diameter, multiplied by the
        factor 1/ro to take ro into account.
        So we use topLeftY _but_ multiplied by -1, as topLeftY is in computer
        coordinates.
    */
    QRectF rectangle = QRectF(topLeftX,-topLeftY,
                              topLeftY, topLeftY);
    rectangle = rectangle.normalized();

    if(pen == thinPen)
    {
        thinArcsPath.arcMoveTo(rectangle, startAngle);
        thinArcsPath.arcTo(rectangle, startAngle, span);
    }
    else
    {
        thickArcsPath.arcMoveTo(rectangle, startAngle);
        thickArcsPath.arcTo(rectangle, startAngle, span);
    }

    // Lower Arc
    /*
        Here the second parameter (top left Y corner) will always be zero.
        The width and length of the rectangle do not vary.
    */
    rectangle.setRect(topLeftX, 0.0,
                      topLeftY, topLeftY);
    rectangle = rectangle.normalized();

    // In the lower arcs, we must multiply by -1 the startAngle and the span
    if(pen == thinPen)
    {
        thinArcsPath.arcMoveTo(rectangle, -startAngle);
        thinArcsPath.arcTo(rectangle, -startAngle, -span);
    }
    else
    {
        thickArcsPath.arcMoveTo(rectangle, -startAngle);
        thickArcsPath.arcTo(rectangle, -startAngle, -span);
    }
}

