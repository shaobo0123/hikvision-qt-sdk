#include "MyCamera.h"

MyCamera::MyCamera()
{
    m_hDevHandle = NULL;
}

MyCamera::~MyCamera()
{
    if (m_hDevHandle)
    {
        MV_CC_DestroyHandle(m_hDevHandle);
        m_hDevHandle = NULL;
    }
}

MV_CC_DEVICE_INFO_LIST MyCamera::m_stDevList;
vector<QString> MyCamera::models;
//查找设备
int MyCamera::findCamera()
{
    memset(&m_stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));    // ch:初始化设备信息列表
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &m_stDevList);
    if(nRet != 0 || m_stDevList.nDeviceNum == 0) {
        qDebug() << "The device was not found";
        return -1;
    }
    models.resize(m_stDevList.nDeviceNum);
    for (unsigned int i = 0; i < m_stDevList.nDeviceNum; i++)
    {
        models[i] = getCameraModels(*m_stDevList.pDeviceInfo[i]);
    }
    return 0;
}

//连接相机
int MyCamera::connectCamera(QString deviceModel)
{
    if(deviceModel.isEmpty()) {
        return -1;
    }

    if(m_isDeviceOpen == true) {
        MV_CC_CloseDevice(m_hDevHandle);
        m_isDeviceOpen = false;
        m_hDevHandle = NULL;
    }

    for (unsigned int i = 0; i < m_stDevList.nDeviceNum; i++)
    {
        if(deviceModel == getCameraModels(*m_stDevList.pDeviceInfo[i])) {
            m_CurrentDevice = m_stDevList.pDeviceInfo[i];
        }
    }

    int nRet = openCamera(m_CurrentDevice);
    if(nRet != 0) return -1;
    return 0;
}

//打开相机
int MyCamera::openCamera(MV_CC_DEVICE_INFO *m_Device){
    if (m_Device == NULL)
    {
        qDebug() << "The camera with the specified name was not found";
        return -1;
    }
    // qDebug() << (char*)m_Device->SpecialInfo.stGigEInfo.chUserDefinedName;//自定义相机名称
    // qDebug() << (char*)m_Device->SpecialInfo.stGigEInfo.chSerialNumber;//相机序列号

    int nRet = MV_CC_CreateHandle(&m_hDevHandle, m_Device);//创建句柄
    if(nRet != 0) {
        qDebug() << "CreateHandle failed";
        return -1;
    }

    nRet = MV_CC_OpenDevice(m_hDevHandle);//打开设备
    if (nRet != 0)
    {
        qDebug() << "OpenDevice failed";
        MV_CC_DestroyHandle(m_hDevHandle);
        m_hDevHandle = NULL;
        m_isDeviceOpen = false;
        return -1;
    }
    qDebug() << "The camera was successfully connected";
    m_isDeviceOpen = true;
    return 0;
}

//获取设备型号
QString MyCamera::getCameraModels() {
    return getCameraModels(*m_CurrentDevice);
}
QString MyCamera::getCameraModels(MV_CC_DEVICE_INFO camera) {
    QString cameraModels;
    // 根据设备类型，提取相机型号
    if (camera.nTLayerType == MV_GIGE_DEVICE)
    {
        // 对于GigE相机
        QString model(reinterpret_cast<const char*>(camera.SpecialInfo.stGigEInfo.chModelName));
        cameraModels = "GigE Camera Model: " + model + "\n";
    }
    else if (camera.nTLayerType == MV_USB_DEVICE)
    {
        // 对于USB相机
        QString model(reinterpret_cast<const char*>(camera.SpecialInfo.stUsb3VInfo.chModelName));
        cameraModels = "USB Camera Model: " + model + "\n";
    }
    return cameraModels;
}

//获取设备IP
QString MyCamera::getCameraIP() {
    QString Ip = nullptr;
    // 根据设备类型，提取相机型号
    if (m_CurrentDevice->nTLayerType == MV_GIGE_DEVICE)
    {
        // 对于GigE相机
        //获取相机的IP地址
        int nIp1,nIp2,nIp3,nIp4;
        nIp1 = ((m_CurrentDevice->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        nIp2 = ((m_CurrentDevice->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        nIp3 = ((m_CurrentDevice->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        nIp4 = (m_CurrentDevice->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
        //ip地址
        Ip = QString("%1.%2.%3.%4").arg(nIp1).arg(nIp2).arg(nIp3).arg(nIp4);
    }
    return Ip;
}

//启动相机采集
int MyCamera::StartGrabbing()
{
    if(isGrabbing == true) {
        return 0;
    }
    int nRet = MV_CC_StartGrabbing(m_hDevHandle);
    if(nRet != 0)
    {
        qDebug() << "The capture failed";
        return -1;
    }else
    {
        //qDebug() << "The capture was successful";
        isGrabbing = true;
        return 0;
    }
}

//停止抓图
int MyCamera::StopGrabbing() {
    if(isGrabbing == false) {
        return 0;
    }
    int nRet = MV_CC_StopGrabbing(m_hDevHandle);
    if (MV_OK != nRet) {
        qDebug() << "Failed to stop grabbing";
        return -1;
    }

    isGrabbing = false;
    return 0;
}

//设置触发模式
int MyCamera::SetTriggerMode(int mode)
{
    StartGrabbing();
    // 设置触发模式：1-打开触发模式 0-关闭触发模式
    int nRet = MV_CC_SetTriggerMode(m_hDevHandle, mode);
    if (nRet != MV_OK)
    {
        qDebug() << "Failed to set trigger mode";
        return -1;
    }
    return 0;
}

//设置软触发模式
int MyCamera::setSoftTrigger()
{
    MV_CC_SetEnumValue(m_hDevHandle, "TriggerMode", MV_TRIGGER_MODE_ON);
    int enumValue = MV_CC_SetEnumValue(m_hDevHandle,"TriggerSource",MV_TRIGGER_SOURCE_SOFTWARE);
    if(enumValue != 0){
        qDebug() << "Failed to set soft trigger";
        return -1;
    }
    int comdValue= MV_CC_SetCommandValue(m_hDevHandle, "TriggerSoftware");
    if(comdValue!=0)
    {
        qDebug() << "set Soft trigger mode is failed";
        return -1;
    }else
    {
        return 0;
    }
}

//软触发
int MyCamera::softTrigger(QImage &cacheQimage)
{
    StartGrabbing();
    setSoftTrigger();
    int ret = GetQImageBuffer(cacheQimage);
    StopGrabbing();
    if (ret != 0) {
        qDebug() << "softTriggerCapture failed";
        return -1;
    }
    return 0;
}

int MyCamera::GetImageBuffer()
{
    int nRet = MV_CC_GetImageBuffer(m_hDevHandle, &stFrameOut, 1000); // 1000 是超时时间（毫秒）
    if (nRet == MV_OK) {
        return 0;
    } else {
        qDebug() << "GetImageBuffer failed";
        return -1;
    }
}

int MyCamera::GetQImageBuffer(QImage &qimageBuffer)
{
    int nRet = GetImageBuffer();
    if (nRet == MV_OK) {
        // 根据图像信息创建对应的 OpenCV Mat 对象
        if (stFrameOut.stFrameInfo.enPixelType == PixelType_Gvsp_Mono8) {
            // 对于单通道灰度图像
            qimageBuffer = QImage(stFrameOut.pBufAddr, stFrameOut.stFrameInfo.nWidth, stFrameOut.stFrameInfo.nHeight, QImage::Format_Grayscale8);
        } else if (stFrameOut.stFrameInfo.enPixelType == PixelType_Gvsp_RGB8_Packed) {
            // 对于 RGB 图像
            qimageBuffer = QImage(stFrameOut.pBufAddr, stFrameOut.stFrameInfo.nWidth, stFrameOut.stFrameInfo.nHeight, QImage::Format_RGB888);
            qimageBuffer = qimageBuffer.convertToFormat(QImage::Format_RGB32); // 可能需要转换格式以便显示
        }
        // 释放图像缓冲区
        nRet = MV_CC_FreeImageBuffer(m_hDevHandle, &stFrameOut);
        return 0;
    } else {
        return -1;
    }
}

int MyCamera::GetMatImageBuffer(Mat &imageMatBuffer)
{
    int nRet = GetImageBuffer();
    if (nRet == MV_OK) {
        // 根据图像信息创建对应的 OpenCV Mat 对象
        if (stFrameOut.stFrameInfo.enPixelType == PixelType_Gvsp_Mono8) {
            // 对于单通道灰度图像
            imageMatBuffer = cv::Mat(stFrameOut.stFrameInfo.nHeight, stFrameOut.stFrameInfo.nWidth, CV_8UC1, stFrameOut.pBufAddr);
        } else if (stFrameOut.stFrameInfo.enPixelType == PixelType_Gvsp_RGB8_Packed) {
            // 对于 RGB 图像
            imageMatBuffer = cv::Mat(stFrameOut.stFrameInfo.nHeight, stFrameOut.stFrameInfo.nWidth, CV_8UC3, stFrameOut.pBufAddr);
            cv::cvtColor(imageMatBuffer, imageMatBuffer, cv::COLOR_RGB2BGR); // 转换为 OpenCV 的 BGR 格式
        }
        // 释放图像缓冲区
        nRet = MV_CC_FreeImageBuffer(m_hDevHandle, &stFrameOut);
        return 0;
    } else {
        return -1;
    }
}

//停止采集
int MyCamera::stopCapture() {
    int nRet = SetTriggerMode(1);
    nRet = StopGrabbing();
    if (MV_OK != nRet) {
        qDebug() << "Failed to stop grabbing";
        return -1;
    }
    return 0;
}

//关闭相机
int MyCamera::closeCamera()
{
    if (m_hDevHandle == NULL)
    {
        qDebug() <<  "No handle, no need to close"; //没有句柄，不用关闭
        return -1;
    }
    stopCapture();
    MV_CC_CloseDevice(m_hDevHandle);
    int nRet = MV_CC_DestroyHandle(m_hDevHandle);
    m_isDeviceOpen = false;
    m_hDevHandle = NULL;
    return nRet;
}

// 设置曝光模式
int MyCamera::setExposure(bool autoExposure, float exposureTime = -1) {
    int nRet;
    if (autoExposure) {
        // 设置为自动曝光模式
        nRet = MV_CC_SetEnumValue(m_hDevHandle, "ExposureAuto", 1); // 假设1表示自动曝光
        if (MV_OK != nRet) {
            qDebug() << "Failed to set auto exposure";
            return -1;
        }
    } else {
        // 设置为手动曝光模式
        nRet = MV_CC_SetEnumValue(m_hDevHandle, "ExposureAuto", 0); // 假设0表示手动曝光
        if (MV_OK != nRet) {
            qDebug() << "Failed to set manual exposure";
            return -1;
        }

        // 设置曝光时间
        if (exposureTime > 0) {
            nRet = MV_CC_SetFloatValue(m_hDevHandle, "ExposureTime", exposureTime);
            if (MV_OK != nRet) {
                qDebug() << "Failed to set exposure time";
                return -1;
            }
        }
    }
    return 0;
}
