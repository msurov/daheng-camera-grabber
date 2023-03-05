#pragma once
#include <gxi/GxIAPI.h>
#include <gxi/DxImageProc.h>
#include "throws.h"
#include <thread>
#include <functional>
#include <opencv2/opencv.hpp>



using GxCamCallback = std::function<bool(cv::Mat const&)>;

/**
 * @brief GXLibrary
*/
class GXLibrary
{
    GXLibrary();

public:
    GXLibrary(GXLibrary const&) = delete;
    GXLibrary(GXLibrary&&) = delete;
    static GXLibrary& instance();
    ~GXLibrary();
};


/**
 * @brief GXCamera
*/
class GXCamera
{
private:
    GX_DEV_HANDLE _devhandle;
    bool _stop;
    int64_t _bufsz;
    std::thread _thread;
    GxCamCallback _callback;

    void main_loop();

public:
    void print_device_info();
    GXCamera(GXLibrary& lib, GxCamCallback&& callback);
    ~GXCamera();
    void stop();
    void run();
};
