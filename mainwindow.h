#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mycamera.h"

#include <QApplication>

#include <QImage>
#include <QImageReader>

#include <QFileDialog>

#include <Qthread>

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>

class CameraWorker;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    MyCamera *m_pcMycamera = nullptr;
    string cameraName; //相机名称
    Mat imageMat; //使用OpenCV接受采集图像
private:
    Ui::MainWindow *ui;

    //连续采集线程
    CameraWorker *worker_camera;
    QThread *thread_camera;

    // CameraCalibration *c;
    std::stringstream print_CameraCalibration; //用于输出标定结果

    QPixmap cachePixmap;
    QPixmap cachePixmap_continousCapture;

    QImage cacheQimage;

    void updateLabelImage();
    bool iscontinousAcquision = false;

signals:
    void stopWorker();
    void logMessage(QString message);

public slots:
    void getCalibrationResult();

private slots:
    void on_pushButton_findDevices_clicked();

    void on_pushButton_link_clicked();

    void on_pushButton_close_clicked();

    void on_pushButton_softTrigger_clicked();

    void on_pushButton_continousAcquision_clicked();

    void continousAcquision();
    void continousAcquisionStop();

    void onCalibrationFinished();

    void onExposureModeChanged();

    void on_pushButton_getResult_clicked();

    //抓拍
    void on_pushButton_capture_clicked();

    void on_pushButton_save_clicked();

    //void on_pushButton_clear1_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_k_clicked();

    void openSettingsDialog();

};

QImage cvMat2QImage(const cv::Mat& mat);

//标定线程
class CalibrationThread : public QThread {
    Q_OBJECT
public:
    CalibrationThread(MainWindow *mainWindow) : mainWindow(mainWindow) {}

signals:
    void calibrationStarted();
    void calibrationFinished();

protected:
    void run() override {
        emit calibrationStarted();
        mainWindow->getCalibrationResult();
        emit calibrationFinished();
    }

private:
    MainWindow *mainWindow;
};

//连续捕获线程
class CameraWorker : public QObject {
    Q_OBJECT
public:
    explicit CameraWorker(MyCamera *camera) : m_pcMycamera(camera) {
        isrunning = false;
    }

signals:
    void continuousCapturimageupdate(const QImage &image);
    void finished();

public slots:
    void continuousCapture() {
        isrunning = true;
        while (isrunning) {
            int ret = m_pcMycamera->GetQImageBuffer(qimg);
            if (ret == 0) {
                emit continuousCapturimageupdate(qimg);
            } else if (ret == -1) {
                emit finished();
                break;
            }
            QThread::msleep(50);
        }
        emit finished();
    }

    void stopContinuousCapture() {
        isrunning = false;
        emit finished();
    }

private:
    MyCamera *m_pcMycamera;
    QImage qimg;
    bool isrunning;
};


#endif // MAINWINDOW_H
