#include "mainwindow.h"
#include "ui_mainwindow.h"
// #include "../Steel/FileIO.h"
// #include "../Steel/FileIO.cpp"
#include <QVBoxLayout>
#include <QCheckBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //ui->plainTextEdit_log->setReadOnly(true);
    //ui->lineEditExposureTime->setReadOnly(true);
    //ui->plainTextEdit_result->setReadOnly(true);

    // 隐藏标签栏
    ui->tabWidget->tabBar()->hide();

    // 连接信号和槽：当下拉框选项改变时，切换页面
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
        ui->tabWidget, &QTabWidget::setCurrentIndex);

    // 曝光选择
    connect(ui->radioButtonAutoExposure, SIGNAL(toggled(bool)), this, SLOT(onExposureModeChanged()));
    connect(ui->radioButtonManualExposure, SIGNAL(toggled(bool)), this, SLOT(onExposureModeChanged()));

    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::on_pushButton_4_clicked);

    QList<QPushButton *> allButtons = ui->groupBox_2->findChildren<QPushButton *>();
    for (QPushButton *button : allButtons) {
        button->setEnabled(false);  // 禁用按钮
    }
    ui->pushButton_close->setEnabled(false);


    // 创建菜单栏
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // 创建菜单
    QMenu* settingsMenu = menuBar->addMenu("Settings");

    // 创建菜单项
    QAction* openSettingsAction = new QAction("File Settings", this);
    settingsMenu->addAction(openSettingsAction);

    // 连接菜单项的点击信号到槽函数
    connect(openSettingsAction, &QAction::triggered, this, &MainWindow::openSettingsDialog);

}

MainWindow::~MainWindow()
{
    if(m_pcMycamera != nullptr) {
        m_pcMycamera->closeCamera();
        delete m_pcMycamera;
        m_pcMycamera = nullptr;
    }
    delete ui;
}

//查找相机
void MainWindow::on_pushButton_findDevices_clicked()
{
    static MyCamera m_findMycamera;
    if(m_findMycamera.findCamera() != 0){
        return;
    }
    // 将相机型号设置到QLineEdit控件中
    ui->label_device->setText(m_findMycamera.getCameraModels(*m_findMycamera.m_stDevList.pDeviceInfo[0]));
}

//连接相机
void MainWindow::on_pushButton_link_clicked()
{
    m_pcMycamera = new MyCamera;
    if(ui->label_device->text().isEmpty()) {
        ui->plainTextEdit_log->appendPlainText("can't find link_device");
        return;
    }
    if(m_pcMycamera->connectCamera(ui->label_device->text()) != 0){
        return;
    }

    ui->pushButton_link->setText("yes!");
    ui->pushButton_link->setEnabled(false);
    QList<QPushButton *> allButtons = ui->groupBox_2->findChildren<QPushButton *>();
    for (QPushButton *button : allButtons) {
        button->setEnabled(true);
    }
    ui->pushButton_close->setEnabled(true);
}

//关闭设备
void MainWindow::on_pushButton_close_clicked()
{
    //关闭设备，释放资源
    int close = m_pcMycamera->closeCamera();
    delete m_pcMycamera;
    m_pcMycamera = nullptr;
    if(close != 0){
        ui->plainTextEdit_log->appendPlainText("Camera shutdown failed");
        return;
    }
    //更新ui
    //ui->label_device->clear();
    ui->pushButton_link->setText("link");
    ui->pushButton_link->setEnabled(true);
    QList<QPushButton *> allButtons = ui->groupBox_2->findChildren<QPushButton *>();
    for (QPushButton *button : allButtons) {
        button->setEnabled(false);  // 禁用按钮
    }
    ui->pushButton_close->setEnabled(false);
}

//更新图像
void MainWindow::updateLabelImage() {
    cachePixmap = QPixmap::fromImage(cacheQimage).scaled(ui->label_image->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_image->setPixmap(cachePixmap);
}

//采集单张图像按钮
void MainWindow::on_pushButton_softTrigger_clicked()
{
    //设置相机软触发
    int ret = m_pcMycamera->softTrigger(cacheQimage);
    if (ret != 0) {
        return;
    }
    updateLabelImage();
}

//连续采集按钮
void MainWindow::on_pushButton_continousAcquision_clicked() {
    if(iscontinousAcquision) {
        continousAcquisionStop();
    } else {
        continousAcquision();
    }
}

//连续采集
void MainWindow::continousAcquision() {
    // 启动连续采集
    int nRet = m_pcMycamera->SetTriggerMode(0);
    if (nRet != 0) {
        return;
    }
    ui->pushButton_softTrigger->setEnabled(false);
    iscontinousAcquision = true;
    ui->pushButton_continousAcquision->setText("停止连续采集");

    // 创建并启动线程
    worker_camera = new CameraWorker(m_pcMycamera);
    thread_camera = new QThread(this);

    worker_camera->moveToThread(thread_camera);

    connect(thread_camera, &QThread::started, worker_camera, &CameraWorker::continuousCapture);
    connect(worker_camera, &CameraWorker::continuousCapturimageupdate, this, [this](const QImage &qimg) {
        cachePixmap_continousCapture = QPixmap::fromImage(qimg).scaled(ui->label_image2->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->label_image2->setPixmap(cachePixmap_continousCapture);
    });

    connect(worker_camera, &CameraWorker::finished, thread_camera, &QThread::quit);
    connect(worker_camera, &CameraWorker::finished, worker_camera, &QObject::deleteLater);
    connect(thread_camera, &QThread::finished, thread_camera, &QThread::deleteLater);

    connect(this, &MainWindow::stopWorker, worker_camera, &CameraWorker::stopContinuousCapture);

    thread_camera->start();
}

//停止采集
void MainWindow::continousAcquisionStop()
{
    m_pcMycamera->stopCapture();
    emit stopWorker(); // 发送停止信号给工作对象
    ui->pushButton_softTrigger->setEnabled(true);
    iscontinousAcquision = false;
    ui->pushButton_continousAcquision->setText("连续采集");
    ui->label_image2->clear();
}

//抓拍图像
void MainWindow::on_pushButton_capture_clicked() {
    int ret = m_pcMycamera->GetQImageBuffer(cacheQimage);
    if (ret != 0) {
        ui->plainTextEdit_log->appendPlainText("CaptureImage failed");
        return;
    }
    updateLabelImage();
}

// 曝光选择
void MainWindow::onExposureModeChanged() {
    if (ui->radioButtonAutoExposure->isChecked()) {
        ui->lineEditExposureTime->setReadOnly(true);
        disconnect(ui->lineEditExposureTime, SIGNAL(returnPressed()), this, SLOT(onExposureModeChanged()));
        m_pcMycamera->setExposure(true, -1); // 启用自动曝光
    } else if (ui->radioButtonManualExposure->isChecked()) {
        ui->lineEditExposureTime->setReadOnly(false);
        connect(ui->lineEditExposureTime, SIGNAL(returnPressed()), this, SLOT(onExposureModeChanged()));
        float manualExposureTime = ui->lineEditExposureTime->text().toFloat(); // 从QLineEdit获取曝光时间
        m_pcMycamera->setExposure(false, manualExposureTime); // 设置手动曝光及其时间
    }
}

//保存图像
void MainWindow::on_pushButton_save_clicked() {
    // 弹出一个对话框，让用户选择保存图像的路径
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr("Images (*.png *.xpm *.jpg)"));

    // 检查用户是否选择了文件
    if (!fileName.isEmpty() && !imageMat.empty()) {
        // 如果用户选择了文件，使用选定的路径保存图像
        cv::imwrite(fileName.toStdString(), imageMat);
    }
}

////清除图像
//void MainWindow::on_pushButton_clear1_clicked() {
//    for(QLabel* label : ui->tabWidget_2->findChildren<QLabel*>()) {
//        label->clear();
//    }
//    imageMat.release();
//}

////上传图像
//void MainWindow::onImageUploadButtonClicked() {
//    QString filePath = QFileDialog::getOpenFileName(this, "Select an image", "", "Images (*.png *.xpm *.jpg *.bmp)");
//    if (!filePath.isEmpty()) {
//        // 使用OpenCV读取图像
//        imageMat = cv::imread(filePath.toStdString());
//        cacheQimage = cvMat2QImage(imageMat);
//
//        // 检查图像是否成功加载
//        if (!imageMat.empty()) {
//            ui->plainTextEdit_log->appendPlainText("Successfully get image locally.");
//            updateLabelImage();
//            ui->plainTextEdit_upload->setPlainText(filePath);
//        } else {
//            // 处理读取图像失败的情况
//        }
//    } else {
//        // 处理用户未选择文件的情况
//    }
//}

//获取标定结果
void MainWindow::getCalibrationResult(){
    // cv::Mat A = FileIO::readIntrinsicMatrixFromFile("D:/code/SteelDefectDetectionProject/data/intrinsic_params.txt");

    // this->print_CameraCalibration.str("");
    // this->print_CameraCalibration.clear();
    // this->print_CameraCalibration << A;
}

// 用于在单独的线程中调用 getCalibrationResult
void MainWindow::on_pushButton_getResult_clicked() {
    if(imageMat.empty()) {
        ui->plainTextEdit_log->appendPlainText("image is empty");
        return;
    }
    // if(ui->plainTextEdit_cellsize->toPlainText() == nullptr || ui->plainTextEdit_f->toPlainText() == nullptr){
    //     ui->plainTextEdit_log->appendPlainText("cellsize or f is empty");
    //     return;
    // }

    // 创建并启动线程
    CalibrationThread *calibrationThread = new CalibrationThread(this);
    connect(calibrationThread, &CalibrationThread::calibrationStarted, this, [this]() {
        ui->plainTextEdit_log->appendPlainText("calibration thread is running");
        ui->plainTextEdit_result->clear();
        ui->pushButton_getResult->setText("running");
        ui->pushButton_getResult->setEnabled(false);
    });
    connect(calibrationThread, &CalibrationThread::calibrationFinished, this, &MainWindow::onCalibrationFinished);
    connect(calibrationThread, &QThread::finished, calibrationThread, &QObject::deleteLater);
    calibrationThread->start();
}

// 添加一个槽处理标定完成
void MainWindow::onCalibrationFinished() {

    QString newText = QString::fromStdString(this->print_CameraCalibration.str());
    ui->plainTextEdit_result->setPlainText(newText);

    ui->pushButton_getResult->setText(QString::fromUtf8("getResult"));
    ui->pushButton_getResult->setEnabled(true);
}

//Mat转QImage函数
QImage cvMat2QImage(const cv::Mat& mat)
{
    // 8位无符号，通道数 = 1
    if(mat.type() == CV_8UC1)
    {
        QImage qimage(mat.cols, mat.rows, QImage::Format_Indexed8);
        // 设置颜色表（用于将颜色索引转换为 qRgb 值）
        qimage.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            qimage.setColor(i, qRgb(i, i, i));
        }
        // 复制输入的 Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = qimage.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return qimage;
    }
    // 8位无符号，通道数 = 3
    else if(mat.type() == CV_8UC3)
    {
        // 复制输入的 Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // 创建与输入 Mat 尺寸相同的 QImage
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        // 复制输入的 Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // 创建与输入 Mat 尺寸相同的 QImage
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        return QImage();
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    // 打开文件对话框选择图片
    QString filePath = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty()) {
        //QMessageBox::warning(this, "警告", "未选择图片");
        return;
    }

    // 加载图片
    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        //QMessageBox::warning(this, "错误", "图片加载失败");
        return;
    }

    // 在label_image3中显示图片（保持比例缩放）
    ui->label_image3->setPixmap(pixmap.scaled(ui->label_image3->size(), Qt::KeepAspectRatio));
}

void MainWindow::on_pushButton_k_clicked() {
    // 打开文件对话框，选择图像文件
    QString fileName = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");

    if (!fileName.isEmpty()) {
        // 加载图像到 QPixmap
        QPixmap pixmap(fileName);

        // 将图像设置到 label_image2
        ui->label_image2->setPixmap(pixmap.scaled(ui->label_image2->size(), Qt::KeepAspectRatio));
    }
}

void MainWindow::openSettingsDialog() {
    // 创建一个对话框
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Settings");

    // 设置对话框的布局
    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // 相机标定保存文件夹路径
    QHBoxLayout* cameraCalibrationLayout = new QHBoxLayout();
    QLabel* cameraCalibrationLabel = new QLabel("Camera Calibration Path:", dialog);
    QLineEdit* cameraCalibrationPathEdit = new QLineEdit(dialog);
    QPushButton* cameraCalibrationBrowseButton = new QPushButton("Browse", dialog);
    cameraCalibrationLayout->addWidget(cameraCalibrationLabel);
    cameraCalibrationLayout->addWidget(cameraCalibrationPathEdit);
    cameraCalibrationLayout->addWidget(cameraCalibrationBrowseButton);
    layout->addLayout(cameraCalibrationLayout);

    // 光平面标定保存文件夹路径
    QHBoxLayout* lightPlaneCalibrationLayout = new QHBoxLayout();
    QLabel* lightPlaneCalibrationLabel = new QLabel("Light Plane Calibration Path:", dialog);
    QLineEdit* lightPlaneCalibrationPathEdit = new QLineEdit(dialog);
    QPushButton* lightPlaneCalibrationBrowseButton = new QPushButton("Browse", dialog);
    lightPlaneCalibrationLayout->addWidget(lightPlaneCalibrationLabel);
    lightPlaneCalibrationLayout->addWidget(lightPlaneCalibrationPathEdit);
    lightPlaneCalibrationLayout->addWidget(lightPlaneCalibrationBrowseButton);
    layout->addLayout(lightPlaneCalibrationLayout);

    // 是否导出点云
    QCheckBox* exportPointCloudCheckBox = new QCheckBox("Export Point Cloud", dialog);
    layout->addWidget(exportPointCloudCheckBox);

    // 点云导出文件夹路径
    QHBoxLayout* pointCloudExportLayout = new QHBoxLayout();
    QLabel* pointCloudExportLabel = new QLabel("Point Cloud Export Path:", dialog);
    QLineEdit* pointCloudExportPathEdit = new QLineEdit(dialog);
    QPushButton* pointCloudExportBrowseButton = new QPushButton("Browse", dialog);
    pointCloudExportLayout->addWidget(pointCloudExportLabel);
    pointCloudExportLayout->addWidget(pointCloudExportPathEdit);
    pointCloudExportLayout->addWidget(pointCloudExportBrowseButton);
    layout->addLayout(pointCloudExportLayout);

    // 保存按钮
    QPushButton* saveButton = new QPushButton("Save", dialog);
    layout->addWidget(saveButton);

    // 连接按钮点击信号到槽函数
    connect(cameraCalibrationBrowseButton, &QPushButton::clicked, [=]() {
        QString path = QFileDialog::getExistingDirectory(dialog, "Select Camera Calibration Path");
        if (!path.isEmpty()) {
            cameraCalibrationPathEdit->setText(path);
        }
        });

    connect(lightPlaneCalibrationBrowseButton, &QPushButton::clicked, [=]() {
        QString path = QFileDialog::getExistingDirectory(dialog, "Select Light Plane Calibration Path");
        if (!path.isEmpty()) {
            lightPlaneCalibrationPathEdit->setText(path);
        }
        });

    connect(pointCloudExportBrowseButton, &QPushButton::clicked, [=]() {
        QString path = QFileDialog::getExistingDirectory(dialog, "Select Point Cloud Export Path");
        if (!path.isEmpty()) {
            pointCloudExportPathEdit->setText(path);
        }
        });

    connect(saveButton, &QPushButton::clicked, [=]() {
        QString cameraCalibrationPath = cameraCalibrationPathEdit->text();
        QString lightPlaneCalibrationPath = lightPlaneCalibrationPathEdit->text();
        bool exportPointCloud = exportPointCloudCheckBox->isChecked();
        QString pointCloudExportPath = pointCloudExportPathEdit->text();

        // 在这里处理保存逻辑
        qDebug() << "Camera Calibration Path:" << cameraCalibrationPath;
        qDebug() << "Light Plane Calibration Path:" << lightPlaneCalibrationPath;
        qDebug() << "Export Point Cloud:" << exportPointCloud;
        qDebug() << "Point Cloud Export Path:" << pointCloudExportPath;

        dialog->close();
        });

    // 显示对话框
    dialog->exec(); // 模态显示对话框
}
