#ifndef MyCamera_H
#define MyCamera_H
#pragma execution_character_set("utf-8")   //设置当前文件为UTF-8编码
#pragma warning( disable : 4819 )    //解决SDK中包含中文问题；忽略C4819错误
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QImage>
#include "MvCameraControl.h"

using namespace std;
using namespace cv;

class MyCamera
{
public:
    MyCamera();
    ~MyCamera();

    static MV_CC_DEVICE_INFO_LIST m_stDevList;
    static vector<QString> models;

    MV_CC_DEVICE_INFO *m_CurrentDevice;

    //查找设备
    static int findCamera();

    // ch:连接相机
    int connectCamera(QString deviceModel);

    //获取设备型号
    QString getCameraModels();
    static QString getCameraModels(MV_CC_DEVICE_INFO camera);

    //获取设备IP
    QString getCameraIP();

    //开启相机采集
    int StartGrabbing();

    //设置触发模式
    int SetTriggerMode(int mode);

    //软出发模式
    int softTrigger(QImage &cacheQimage);

    //读取相机中的图片
    int GetQImageBuffer(QImage &qimageBuffer);
    int GetMatImageBuffer(Mat &imageMatBuffer);

    //停止采集
    int StopGrabbing();
    int stopCapture();

    //关闭相机
    int closeCamera();

    //设置曝光
    int setExposure(bool autoExposure, float exposureTime);
private:
    void* m_hDevHandle;
    MV_FRAME_OUT stFrameOut;
    bool m_isDeviceOpen = false;
    bool isGrabbing = false;

    //读取相机中的图片
    int GetImageBuffer();
    //发送软触发
    int setSoftTrigger();
    //打开相机
    int openCamera(MV_CC_DEVICE_INFO *m_Device);

public: unsigned char* m_pBufForSaveImage;
    // 用于保存图像的缓存
    unsigned int m_nBufSizeForSaveImage;
    unsigned char* m_pBufForDriver;
    // 用于从驱动获取图像的缓存
    unsigned int m_nBufSizeForDriver;
};

#endif // MYCAMERA_H
