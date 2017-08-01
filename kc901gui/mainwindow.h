#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
//#include <qwt_plot.h>

class QTimer;
class QSettings;
class QwtPlotCurve;
class QElapsedTimer;
class QwtPlotZoomer;
class KCScaleWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QTcpSocket * _pSocket;
    QString senddata;
    QString receivedata;
    QByteArray cmddata;
    int mesmode = 0;

protected:
    void closeEvent(QCloseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *event);

public slots:
    void VSWR(qreal cent, qreal span, int pts);
    void RI(qreal cent, qreal span, int pts);
    void S21(qreal cent, qreal span, int pts);

private slots:
    void on_ConnectpushButton_clicked();

    //void on_whatpushButton_clicked();

    void on_SendpushButton_clicked();

    void on_ClosepushButton_clicked();
    void readTcpData();

    void on_ControlpushButton_clicked();

    void on_LocalpushButton_2_clicked();

    void on_S11initpushButton_clicked();

    void on_vswrmespushButton_clicked();

    void connectSuccess();

   // void on_vswrmespushButton_clicked();

    void startReceive();
    void receiveTimeout();

    void displayS11VSWR(QVector<qreal> freq, QVector<qreal> vswr);
    //void displayS11MA(QVector<qreal> freq, QVector<qreal> mag, QVector<qreal>phase);
    void displayS11RI(QVector<qreal> freq, QVector<QPointF> ri);
    void displayS21(QVector<qreal> freq, QVector<qreal> lose);
    //void displayS11MA(QVector<qreal> freq, QVector<qreal> ma);

    void refinePlot();
    void autoRefine();

    void on_history_doubleClicked(const QModelIndex &index);
    void on_checkBox_toggled(bool checked);
    void on_RLMes_clicked();
    void on_S21initpushButton_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *receiveTimer;
    QElapsedTimer *receiveElapsed;
    QSettings *cfg;
    QwtPlotCurve *s11curve, *s21curve;
    QwtPlotZoomer *zoomer;
    bool autoscaleAndZoomReset;
    KCScaleWidget *bottomScaleWidget;
    bool autoRefineEnable;

    bool parseCentSpanPts(qreal *cent, qreal *span, int *pts);
};

#endif // MAINWINDOW_H
