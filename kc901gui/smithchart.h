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
#pragma once

#include <QWidget>
#include <QPen>
#include <QVector>

// Forward declarations
class QPaintEvent;
class QPainterPath;
class QPoint;

/// Smith chart graphic class
/**
Shows a Smith chart and is able to graphic data into it.
*/

class SmithChart : public QWidget
{
	Q_OBJECT
    Q_PROPERTY(qreal padding READ padding WRITE setPadding NOTIFY paddingChanged)

public:
	SmithChart(QWidget * parent = 0);
	~SmithChart();

	/// Clear the data
	void clear();

	void draw(QPainter * painter);
	/// Sets the data to be drawn
	/**
	This function takes impedances already normalized.
	The data will be stored in a row, and it cannot be deleted.
	*/
    void setZ(const double & real, const double & imaginary);
	/// Sets the data to be drawn
	/**
	Same as above but we can add data in the form of a vector.
	*/
    void setZ(const QVector<QPointF> ZVector);

    void setReflection(const QVector<QPointF> L);

    void setStyle(QBrush background, QColor scale = QColor(Qt::black), QPen datapoint = QPen(Qt::red, 4.0, Qt::SolidLine, Qt::RoundCap), QPen dataline = QPen(Qt::blue, 1.0));

    qreal padding();

signals:
    void paddingChanged();

public slots:

	/// Show interpolated line
	/**
	Draws a line between the data points
	*/
	void showLine(bool show = true);
    void setPadding(qreal padding);

protected:
	void paintEvent(QPaintEvent *event);

private:
	/// Calculates the inside arcs and loads it to the painter paths
	void calculateInsideArcs();

	/// Calculate the coordinates of the impedance
	QPointF calculateZCoordinates(const double & real, const double & imaginary);

    /// Calculate the coordinates of the impedance
    QPointF calculateXY(const double & real, const double & imaginary);

	/**
		Draws Z = constant circles
		Z is the constant value to be drawn
		angle is the angle between 3 o'clock and the beggining of the arc
		xPosition is the position of the most left side
	*/
	void drawConstantZArc(QPen pen, double z, double angle, double span);

	/**
		Draws Ro = constant circles
		ro is the constant value to be drawn
		spawn is the longitude of the arc
	 FIXME
	*/
	void drawConstantRoArc(QPen pen, double ro,
	                       double startAngle, double span);

	/// The data vector
	QVector<QPointF> dataVector;

	/// A done flag
	bool arcsCalculated;

	/// Show the interpolated line between the data points
	bool showInterpolatedLine;

	/// Pen for the text
	QPen textPen;
	/// Thick pen
	QPen thickPen;
	/// Thin pen
	QPen thinPen;
	/// Pen for the data points
	QPen pointDataPen;
	/// Pen for the lines interpolating the data points
	QPen lineDataPen;

	/// Path for the thin arcs
	QPainterPath thinArcsPath;
	/// Path for the thick arcs
	QPainterPath thickArcsPath;

    QBrush backgroundBrush;
    QColor scaleColor;
    QFont textFont;

    qreal m_padding;
};
