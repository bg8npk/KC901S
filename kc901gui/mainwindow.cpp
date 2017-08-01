#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QSettings>
#include "qwt_plot_curve.h"
#include "qwt_point_data.h"
#include "qwt_curve_fitter.h"
#include "qwt_legend.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_engine.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QStringList>
#include "qwt_plot_zoomer.h"
#include "qwt_plot_magnifier.h"
#include "qwt_plot_panner.h"
#include "qwt_scale_widget.h"
#include "kcscalewidget.h"
#include "qwt_picker_machine.h"
#include "smithchart.h"
#include "cmath"

#define DARKSTYLE
//#define BRIGHTSTYLE
#define KC901V_FIX

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->centralWidget->hide();
    showMaximized();

    cfg = new QSettings("kc901.plist", QSettings::IniFormat, this);
    cfg->setIniCodec("UTF-8");
    restoreState(cfg->value("windowstate").toByteArray());

    ui->AddresslineEdit->setText(cfg->value("ip", "192.168.199.91").toString());
    ui->PortlineEdit->setText(QString("%1").arg(cfg->value("port", 1901).toInt()));

    qreal cent = (3e9+250e3)/2;
    qreal span = 3e9-250e3;
    int pts = 100;
    ui->CentlineEdit->setText(cfg->value("s11/cent", QString("%1").arg(cent)).toString());
    ui->SpanlineEdit->setText(cfg->value("s11/span", QString("%1").arg(span)).toString());
    ui->PointlineEdit->setText(cfg->value("s11/pts", QString("%1").arg(pts)).toString());

    receiveTimer = new QTimer();
    connect(receiveTimer, SIGNAL(timeout()), this, SLOT(receiveTimeout()));

    receiveElapsed = new QElapsedTimer();

    _pSocket = new QTcpSocket( this );

    //replace xbottom scale widget
    bottomScaleWidget = new KCScaleWidget(QwtScaleDraw::BottomScale, ui->plot);

    s11curve = new QwtPlotCurve(trUtf8("S11"));
    s21curve = new QwtPlotCurve(trUtf8("S21"));
    s11curve->setVisible(false);
    s21curve->setVisible(false);
    s11curve->attach(ui->plot);
    s21curve->attach(ui->plot);

    ui->plot->setCanvasBackground(QBrush(Qt::white));
    ui->plot->axisScaleEngine(QwtPlot::xBottom)->setMargins(0.0,0.0);
    ui->plot->setAxisMaxMajor(QwtPlot::xBottom, 10);
    ui->plot->setAxisMaxMinor(QwtPlot::xBottom, 10);
    ui->plot->setAxisMaxMajor(QwtPlot::yLeft, 10);
    ui->plot->setAxisMaxMinor(QwtPlot::yLeft, 10);

    QwtSplineCurveFitter *myCurveFitter = new QwtSplineCurveFitter();
    myCurveFitter->setFitMode(QwtSplineCurveFitter::Spline);
    s11curve->setCurveAttribute(QwtPlotCurve::Fitted);
    s11curve->setCurveFitter(myCurveFitter);
    s11curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    QwtPlotGrid *grid = new QwtPlotGrid();
#ifdef BRIGHTSTYLE
    ui->plot->setStyleSheet("background: white;"
                            "color: black;");
    s11curve->setPen( QPen(
        QColor(cfg->value("s11/linecolor",QColor(Qt::blue).rgb()).toUInt()),
        cfg->value("s11/linewidth",1.0).toFloat()
    ));
    grid->setMajorPen( QPen(
        QColor(cfg->value("background/majorcolor",QColor(255,183,84).rgb()).toUInt()),
        cfg->value("background/majorwidth", 1.0).toFloat()
    ));
    grid->setMinorPen( QPen(
        QColor(cfg->value("background/minorcolor",QColor(255,220,200).rgb()).toUInt()),
        cfg->value("background/minorpen", 0.5).toFloat()
    ));
#endif
#ifdef DARKSTYLE
    ui->plot->setStyleSheet("background: black;"
                            "color: white;");
    s11curve->setPen( QPen(
        QColor(cfg->value("s11/linecolor",QColor(0,255,230).rgb()).toUInt()),
        cfg->value("s11/linewidth",1.0).toFloat()
    ));
    s21curve->setPen( QPen(
        QColor(cfg->value("s21/linecolor",QColor(0,255,230).rgb()).toUInt()),
        cfg->value("s21/linewidth",1.0).toFloat()
    ));
    grid->setMajorPen( QPen(
        QColor(cfg->value("background/majorcolor",QColor(40,40,40).rgb()).toUInt()),
        cfg->value("background/majorwidth", 1.0).toFloat()
    ));
    grid->setMinorPen( QPen(
        QColor(cfg->value("background/minorcolor",QColor(20,20,20).rgb()).toUInt()),
        cfg->value("background/minorpen", 0.5).toFloat()
    ));
#endif

    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->attach(ui->plot);

    QwtLegend *legend = new QwtLegend();
    ui->plot->insertLegend(legend, QwtPlot::BottomLegend);

    zoomer = new QwtPlotZoomer(ui->plot->canvas());
#ifdef BRIGHTSTYLE
#endif
#ifdef DARKSTYLE
    zoomer->setRubberBandPen(QPen(Qt::white));
    zoomer->setTrackerPen(QPen(Qt::white));
#endif
    zoomer->setMousePattern(QwtEventPattern::MouseSelect1, Qt::RightButton);
    zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::NoButton);
    zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::NoButton);
    zoomer->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Space);
    //connect(zoomer, SIGNAL(zoomed(QRectF)), this, SLOT(refinePlot()));

    QwtPlotMagnifier *magnifier = new QwtPlotMagnifier(ui->plot->canvas());
    //magnifier->setWheelFactor
    magnifier->setMouseButton(Qt::NoButton, Qt::NoModifier);

    QwtPlotPanner *horzPanner = new QwtPlotPanner(ui->plot->canvas());
    horzPanner->setOrientations(Qt::Horizontal);
    horzPanner->setMouseButton(Qt::MidButton);

    QwtPlotPanner *panner = new QwtPlotPanner(ui->plot->canvas());
    panner->setMouseButton(Qt::LeftButton);

    QwtPlotPicker *picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
            QwtPlotPicker::VLineRubberBand, QwtPicker::AlwaysOn,
            ui->plot->canvas());
    picker->setStateMachine(new QwtPickerClickPointMachine());
    picker->setRubberBandPen(QColor(Qt::red));
    picker->setTrackerPen(QColor(Qt::green));

    ui->plot->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating,true);
    connect(bottomScaleWidget,  SIGNAL(scaleChanged()), this, SLOT(autoRefine()));
    connect(ui->plot->axisWidget(QwtPlot::xBottom), SIGNAL(scaleDivChanged()), this, SLOT(autoRefine()));

    ui->smith->setStyle(
        QBrush(Qt::black),
        QColor(90,130,125),
        //QColor(0,150,50),
        //QColor(80, 120, 150),
        QPen(QColor(Qt::yellow), 1.5),
        QPen(QColor(Qt::yellow), 1.5)
    );
    ui->smith->showLine();
    ui->smith->setPadding(0.05);

    autoRefineEnable = true;
    autoscaleAndZoomReset = true;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete receiveTimer;
    delete receiveElapsed;
    delete cfg;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    cfg->setValue("ip", ui->AddresslineEdit->text());
    cfg->setValue("port", ui->PortlineEdit->text().toInt());
    cfg->setValue("windowstate", saveState());
    cfg->setValue("s11/cent", ui->CentlineEdit->text());
    cfg->setValue("s11/span", ui->SpanlineEdit->text());
    cfg->setValue("s11/pts", ui->PointlineEdit->text());
    Q_UNUSED(event);
}

void MainWindow::VSWR(qreal cent, qreal span, int pts)
{
    if(cent == 0 || span == 0 || pts == 0) return;
//    if(isnan(cent) || isinf(cent) || isnan(span) || isinf(span)) return;
    startReceive();
    if( _pSocket->isWritable() ) {
#ifdef KC901V_FIX
        QByteArray cmd = QString("$S11,run,calon,vswr,%1,CS,%2,%3\n").arg(pts).arg(cent,0,'f',0).arg(span,0,'f',0).toLatin1();
#else
        QByteArray cmd = QString("$S11,run,calon,vswr,point=%1,CS,cent=%2,span=%3\n").arg(pts).arg(cent,0,'f',0).arg(span,0,'f',0).toLatin1();
#endif
        _pSocket->write(cmd);
        qDebug() << cmd;
    }
}


void MainWindow::RI(qreal cent, qreal span, int pts)
{
    if(cent == 0 || span == 0 || pts == 0) return;
//    if(isnan(cent) || isinf(cent) || isnan(span) || isinf(span)) return;
    startReceive();
    if( _pSocket->isWritable() ) {
#ifdef KC901V_FIX
        QByteArray cmd = QString("$S11,run,calon,ri,%1,CS,%2,%3\n").arg(pts).arg(cent,0,'f',0).arg(span,0,'f',0).toLatin1();
#else
        QByteArray cmd = QString("$S11,run,calon,ri,point=%1,CS,cent=%2,span=%3\n").arg(pts).arg(cent,0,'f',0).arg(span,0,'f',0).toLatin1();
#endif
        _pSocket->write(cmd);
        qDebug() << cmd;
    }
}


void MainWindow::S21(qreal cent, qreal span, int pts)
{
    if(cent == 0 || span == 0 || pts == 0) return;
//    if(isnan(cent) || isinf(cent) || isnan(span) || isinf(span)) return;
    startReceive();
    if( _pSocket->isWritable() ) {
#ifdef KC901V_FIX
        QByteArray cmd = QString("$S21,run,calon,lowlo,%1,CS,%2,%3\n").arg(pts).arg(cent,0,'f',0).arg(span,0,'f',0).toLatin1();
#else
        QByteArray cmd = QString("$S21,run,calon,lowlo,point=%1,CS,cent=%2,span=%3\n").arg(pts).arg(cent,0,'f',0).arg(span,0,'f',0).toLatin1();
#endif
        _pSocket->write(cmd);
        qDebug() << cmd;
    }
}


void MainWindow::on_ConnectpushButton_clicked()
{
    QString address;
    QString port;

    address = ui->AddresslineEdit->text();
    port = ui->PortlineEdit->text();
    int PORTnumber = port.toInt();


    _pSocket->connectToHost(address, PORTnumber);
    connect( _pSocket, SIGNAL(readyRead()), SLOT(readTcpData()) );
    connect(_pSocket,SIGNAL(connected()),SLOT(connectSuccess()) );
}

void MainWindow::connectSuccess()
{
    ui->statusBar->showMessage(trUtf8("连接成功"));
}

void MainWindow::startReceive()
//clear receive buffer and start receiver timer
{
    receivedata.clear();
    receiveElapsed->start();
    receiveTimer->start(1000);
}

void MainWindow::receiveTimeout()
{
    ui->statusBar->showMessage(trUtf8("数据接收超时"));
}

void MainWindow::displayS11VSWR(QVector<qreal> freq, QVector<qreal> vswr)
{
    s11curve->setData(new QwtPointArrayData(freq, vswr));
    s11curve->setVisible(true);
    //s21curve->setVisible(false);
    if(autoscaleAndZoomReset)
    {
        //change axes to fit data
        ui->plot->setAxisAutoScale(QwtPlot::yLeft);
        ui->plot->setAxisAutoScale(QwtPlot::xBottom);
        //set zoomer initial state to current axes
        zoomer->setZoomBase();
        autoscaleAndZoomReset = false;
    }
    else
    {
        ui->plot->setAxisAutoScale(QwtPlot::yLeft, false);
        ui->plot->setAxisAutoScale(QwtPlot::xBottom, false);
        ui->plot->replot();
    }
}


void MainWindow::displayS11RI(QVector<qreal> freq, QVector<QPointF> ri)
{
    ui->smith->clear();
    ui->smith->setReflection(ri);
}

void MainWindow::displayS21(QVector<qreal> freq, QVector<qreal> lose)
{
    s21curve->setData(new QwtPointArrayData(freq, lose));
    s21curve->setVisible(true);
    //s21curve->setVisible(false);
    if(autoscaleAndZoomReset)
    {
        //change axes to fit data
        ui->plot->setAxisAutoScale(QwtPlot::yLeft);
        ui->plot->setAxisAutoScale(QwtPlot::xBottom);
        //set zoomer initial state to current axes
        zoomer->setZoomBase();
        autoscaleAndZoomReset = false;
    }
    else
    {
        ui->plot->setAxisAutoScale(QwtPlot::yLeft, false);
        ui->plot->setAxisAutoScale(QwtPlot::xBottom, false);
        ui->plot->replot();
    }
}




void MainWindow::refinePlot()
{
   QwtInterval interval = ui->plot->axisInterval(QwtPlot::xBottom);
    qreal cent = (interval.maxValue() + interval.minValue()) / 2;
   qreal span = interval.width();
    /*
  int pts = ui->plot->canvas()->width();
   if(pts < 10) pts = 10;
    else if(pts > 1000) pts = 1000;
    */
    int pts = ui->PointlineEdit->text().toInt();
    RI(cent, span, pts);
}

void MainWindow::autoRefine()
{
    if(autoRefineEnable)
        refinePlot();
}

void MainWindow::readTcpData()
{
    QByteArray newdata = _pSocket->readAll();
    if(newdata.isEmpty()) return;
    receivedata.append(newdata);
    receiveTimer->start(1000);
    if(receivedata.contains("$end"))
    {
        receiveTimer->stop();

        QStringList list = receivedata.split(QRegExp("[$\n]"));
        list.removeAll("");

        if(list[0] == "start,s11,vswr" && list[list.size()-1] == "end")
        {
            //get s11 vswr
            list.removeFirst();
            list.removeLast();
            QVector<qreal> freq(list.size());
            QVector<qreal> vswr(list.size());
            for(int i = 0; i < list.size(); i++)
            {
                QStringList sample = list[i].split(',');
                if(sample.size() < 2) continue;
                freq[i] = sample[0].toDouble();
                vswr[i] = sample[1].toDouble();
                //qDebug() << QPointF(freq[i], vswr[i]);
            }
            qint64 deltaT = receiveElapsed->elapsed();
            ui->statusBar->showMessage(QString(trUtf8("VSWR:采集到%1数据点,耗时%2ms")).arg(list.size()).arg(deltaT));
            displayS11VSWR(freq, vswr);
        }

        if(list[0] == "start,s21" && list[list.size()-1] == "end")
        {
            //get s21 vswr
            list.removeFirst();
            list.removeLast();
            QVector<qreal> freq(list.size());
            QVector<qreal> s21(list.size());
            for(int i = 0; i < list.size(); i++)
            {
                QStringList sample = list[i].split(',');
                if(sample.size() < 3) continue;
                freq[i] = sample[0].toDouble();
                s21[i] = sample[1].toDouble();
            }
            qint64 deltaT = receiveElapsed->elapsed();
            ui->statusBar->showMessage(QString(trUtf8("S21:采集到%1数据点,耗时%2ms")).arg(list.size()).arg(deltaT));
            displayS21(freq, s21);
        }



        if(list[0] == "start,s11,ri" && list[list.size()-1] == "end")
        {
            //get s11 rl
            list.removeFirst();
            list.removeLast();
            QVector<qreal> freq(list.size());
            QVector<QPointF> s11(list.size());
            QVector<qreal> S11dB(list.size());
            QVector<qreal> S11VSWR(list.size());
            for(int i = 0; i < list.size(); i++)
            {
                QStringList sample = list[i].split(',');
                if(sample.size() < 3) continue;
                freq[i] = sample[0].toDouble();
                s11[i] = QPointF(sample[1].toDouble(), sample[2].toDouble());
                S11dB[i] = 20*log10(sqrt(pow(s11[i].rx(),2) + pow(s11[i].ry(),2)));
                S11VSWR[i] = (1+sqrt(pow(s11[i].rx(),2) + pow(s11[i].ry(),2)))/(1-sqrt(pow(s11[i].rx(),2) + pow(s11[i].ry(),2)));
            }
            qint64 deltaT = receiveElapsed->elapsed();
            ui->statusBar->showMessage(QString(trUtf8("S11:采集到%1数据点,耗时%2ms")).arg(list.size()).arg(deltaT));
            displayS11RI(freq, s11);
            if(mesmode == 1){
                displayS11VSWR(freq, S11dB);
            }
            if(mesmode == 0){
                displayS11VSWR(freq, S11VSWR);
            }
            if(mesmode == 2){
                displayS21(freq, S11VSWR);
            }

        }

        if(list[0] == "start,id" && list[list.size()-1] == "end")
        {
            ui->statusBar->showMessage(QString(trUtf8("申请控制成功, 目标设备序列号%1").arg(list[1])));
        }

    }
    ui->label->setPlainText(receivedata);
}

void MainWindow::on_SendpushButton_clicked()
{
    senddata = ui->CommondlineEdit->text();
    cmddata = senddata.toUtf8();
    startReceive();
    if( _pSocket->isWritable() ) {
        _pSocket->write("$" + cmddata + "\n" );
    }

}

void MainWindow::on_ClosepushButton_clicked()
{
    _pSocket->close();
}

void MainWindow::on_ControlpushButton_clicked()
{
    if( _pSocket->isWritable() ) {
        _pSocket->write("C");
        startReceive();
    }
}

void MainWindow::on_LocalpushButton_2_clicked()
{
    if( _pSocket->isWritable() ) {
        _pSocket->write("$local\n");
    }
}

void MainWindow::on_S11initpushButton_clicked()
{
    autoscaleAndZoomReset = true;
    _pSocket->write("$S21,stop\n");

    if( _pSocket->isWritable() ) {
        _pSocket->write("$S11,init\n");
    }

#ifdef KC901V_FIX
    qreal cent = 3450e6;
    qreal span = 6800e6;
    int pts = 100;
#else
    qreal cent = (3e9+250e3)/2;
    qreal span = 3e9-250e3;
    int pts = 100;
#endif

    ui->CentlineEdit->setText(QString("%1").arg(cent));
    ui->SpanlineEdit->setText(QString("%1").arg(span));
    ui->PointlineEdit->setText(QString("%1").arg(pts));

    VSWR(cent, span, pts);
}

void MainWindow::on_S21initpushButton_clicked()
{
    _pSocket->write("$S11,stop\n");
    autoscaleAndZoomReset = true;

    if( _pSocket->isWritable() ) {
        _pSocket->write("$S21,init\n");
    }

#ifdef KC901V_FIX
    qreal cent = 3450e6;
    qreal span = 6800e6;
    int pts = 100;
#else
    qreal cent = (3e9+250e3)/2;
    qreal span = 3e9-250e3;
    int pts = 100;
#endif

    ui->CentlineEdit->setText(QString("%1").arg(cent));
    ui->SpanlineEdit->setText(QString("%1").arg(span));
    ui->PointlineEdit->setText(QString("%1").arg(pts));

    S21(cent, span, pts);

    mesmode = 2; //s21 mode

}

void MainWindow::on_RLMes_clicked()
{
    qreal cent, span;
    int pts;
    bool convert_ok = parseCentSpanPts(&cent, &span, &pts);
    if(!convert_ok) return;

    autoscaleAndZoomReset = true;
    ui->history->insertItem(0,QString("C=%1,SP=%2,%3pts").arg(cent).arg(span).arg(pts));

    RI(cent, span, pts);
    mesmode = 1; //return loss mesmode
}

void MainWindow::on_vswrmespushButton_clicked()
{
    qreal cent, span;
    int pts;
    bool convert_ok = parseCentSpanPts(&cent, &span, &pts);
    if(!convert_ok) return;

    autoscaleAndZoomReset = true;
    ui->history->insertItem(0,QString("C=%1,SP=%2,%3pts").arg(cent).arg(span).arg(pts));

    RI(cent, span, pts);
    mesmode = 0; //vswr mesmode
}

void MainWindow::on_history_doubleClicked(const QModelIndex &index)
{
    QString str = index.data().toString();
    if(!str.startsWith("C=")) return;
    QStringList args = str.split(',');
    if(args.size() < 3) return;

    qreal cent = args.at(0).mid(2).toDouble();
    qreal span = args.at(1).mid(3).toDouble();
    int pts = args.at(2).mid(0,args.at(2).size() - 3).toInt();

    RI(cent, span, pts);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(ui->plot->frameGeometry().contains(event->pos()))
    {
        qDebug() << event->pos();
        refinePlot();
    }
}

void MainWindow::on_checkBox_toggled(bool checked)
{
    autoRefineEnable = checked;
}

bool MainWindow::parseCentSpanPts(qreal *cent, qreal *span, int *pts)
{
    bool convert_ok;
    if(!cent || !span || !pts) return false;
    *cent = ui->CentlineEdit->text().toDouble(&convert_ok);
    if(!convert_ok) {
        ui->statusBar->showMessage(QString(trUtf8("中心频率参数错误")));
        return false;
    }
    *span = ui->SpanlineEdit->text().toDouble(&convert_ok);
    if(!convert_ok) {
        ui->statusBar->showMessage(QString(trUtf8("扫宽参数错误")));
        return false;
    }
    *pts = ui->PointlineEdit->text().toInt(&convert_ok);
    if(!convert_ok) {
        ui->statusBar->showMessage(QString(trUtf8("扫宽参数错误")));
        return false;
    }
    return true;
}
