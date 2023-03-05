#include <optional>
#include "gxcam.h"

std::string get_cam_status_desc()
{
    GX_STATUS errcode;
    size_t size = 0;
    GX_STATUS status = GX_STATUS_SUCCESS;

    status = GXGetLastError(&errcode, NULL, &size);
    if(status != GX_STATUS_SUCCESS)
        return "";
    
    std::string errdesc(size, '\0');
    status = GXGetLastError(&errcode, &errdesc[0], &size);
    if (status != GX_STATUS_SUCCESS)
        return "Error when calling GXGetLastError";
    return errdesc;
}

std::optional<std::string> get_device_str_param(GX_DEV_HANDLE devhandle, GX_FEATURE_ID_CMD par_id)
{
    size_t sz = 0;
    GX_STATUS status;
    status = GXGetStringLength(devhandle, par_id, &sz);
    if (status != GX_STATUS_SUCCESS)
        return {};
    std::string value(sz, '\0');
    status = GXGetString(devhandle, par_id, &value[0], &sz);
    if (status != GX_STATUS_SUCCESS)
        return {};
    value.resize(sz);
    return {value};
}

std::string get_device_info(GX_DEV_HANDLE devhandle)
{
    std::stringstream ss;
    format_args(ss, 
        "Vendor: ", get_device_str_param(devhandle, GX_STRING_DEVICE_VENDOR_NAME).value_or("undef"), "\n"
        "Model: ", get_device_str_param(devhandle, GX_STRING_DEVICE_MODEL_NAME).value_or("undef"), "\n"
        "Serial: ", get_device_str_param(devhandle, GX_STRING_DEVICE_SERIAL_NUMBER).value_or("undef"), "\n"
        "Version: ", get_device_str_param(devhandle, GX_STRING_DEVICE_VERSION).value_or("undef"), "\n"
    );
    return ss.str();
}

template <typename T>
inline bool set_device_property(GX_DEV_HANDLE devhandle, GX_FEATURE_ID_CMD feature_id, T value)
{
    GX_STATUS status;
    if constexpr (std::is_enum_v<T>) {
        status = GXSetEnum(devhandle, feature_id, value);
    } else if constexpr (std::is_integral_v<T>) {
        status = GXSetInt(devhandle, feature_id, value);
    }
    return status == GX_STATUS_SUCCESS;
}

template <typename T>
inline bool get_device_property(GX_DEV_HANDLE devhandle, GX_FEATURE_ID_CMD feature_id, T& value)
{
    GX_STATUS status;
    if constexpr (std::is_enum_v<T>) {
        status = GXGetInt(devhandle, feature_id, &value);
    } else if constexpr (std::is_integral_v<T>) {
        status = GXGetInt(devhandle, feature_id, &value);
    }
    return status == GX_STATUS_SUCCESS;
}

inline bool is_feature_implemented(GX_DEV_HANDLE devhandle, GX_FEATURE_ID_CMD feature_id)
{
    bool isimpl = false;
    GX_STATUS status = GXIsImplemented(devhandle, GX_DS_INT_STREAM_TRANSFER_SIZE, &isimpl);
    if(status != GX_STATUS_SUCCESS)
        throw_runtime_error("GXIsImplemented failed ", get_cam_status_desc());
    return isimpl;
}

GXLibrary::GXLibrary()
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    status = GXInitLib();
    if(status != GX_STATUS_SUCCESS)
        throw_runtime_error("GXInitLib failed ", get_cam_status_desc());
}

GXLibrary& GXLibrary::instance()
{
    static GXLibrary value;
    return value;
}

GXLibrary::~GXLibrary() { GXCloseLib(); }


void GXCamera::print_device_info()
{
    auto info = get_device_info(_devhandle);
    std::cout << info << std::endl;
}

GXCamera::GXCamera(GXLibrary& lib, GxCamCallback&& callback) :
    _callback(std::forward<GxCamCallback>(callback))
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    uint32_t devnum = 0;
    status = GXUpdateDeviceList(&devnum, 1000);
    if(status != GX_STATUS_SUCCESS)
        throw_runtime_error("GXUpdateDeviceList failed ", get_cam_status_desc());
    if(devnum <= 0)
        throw_runtime_error("No connected device found");

    status = GXOpenDeviceByIndex(1, &_devhandle);
    if(status != GX_STATUS_SUCCESS)
        throw_runtime_error("GXOpenDeviceByIndex failed ", get_cam_status_desc());

    if (!get_device_property(_devhandle, GX_INT_PAYLOAD_SIZE, _bufsz))
        throw_runtime_error("GXSetAcqusitionBufferNumber failed ", get_cam_status_desc());

    if (!set_device_property(_devhandle, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS))
        throw_runtime_error("Can't set property GX_ENUM_ACQUISITION_MODE", get_cam_status_desc());

    if (!set_device_property(_devhandle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF))
        throw_runtime_error("Can't set property GX_ENUM_TRIGGER_MODE", get_cam_status_desc());

    uint64_t nbuffers = 1;
    status = GXSetAcqusitionBufferNumber(_devhandle, nbuffers);
    if(status != GX_STATUS_SUCCESS)
        throw_runtime_error("GXSetAcqusitionBufferNumber failed ", get_cam_status_desc());

    if(is_feature_implemented(_devhandle, GX_DS_INT_STREAM_TRANSFER_SIZE))
        set_device_property(_devhandle, GX_DS_INT_STREAM_TRANSFER_SIZE, 64 * 1024);

    if(is_feature_implemented(_devhandle, GX_DS_INT_STREAM_TRANSFER_NUMBER_URB))
        set_device_property(_devhandle, GX_DS_INT_STREAM_TRANSFER_NUMBER_URB, 64);
}

GXCamera::~GXCamera()
{
    stop();

    if (_devhandle) {
        GXCloseDevice(_devhandle);
        _devhandle = 0;
    }
}

void GXCamera::main_loop()
{
    PGX_FRAME_BUFFER framebuf = NULL;
    uint32_t nframes = 0;
    _stop = false;
    while(!_stop)
    {
        GX_STATUS status = GXDQBuf(_devhandle, &framebuf, 1000);
        if (status == GX_STATUS_TIMEOUT)
            continue;

        if(status != GX_STATUS_SUCCESS)
        {
            std::cerr << "Can't get frame: " << get_cam_status_desc() << std::endl;
            break;
        }

        if(framebuf->nStatus != GX_FRAME_STATUS_SUCCESS)
        {
            std::cerr << "Can't get frame: " << framebuf->nStatus << std::endl;
            break;
        }

        // std::cout << framebuf->nHeight << " " << framebuf->nWidth << " " << _bufsz << " " << framebuf->pImgBuf << std::endl;

        cv::Mat raw(framebuf->nHeight, framebuf->nWidth, CV_8U, framebuf->pImgBuf);
        _callback(raw);

        status = GXQBuf(_devhandle, framebuf);
        if(status != GX_STATUS_SUCCESS)
        {
            std::cerr << "Can't free frame: " << get_cam_status_desc() << std::endl;
            break;
        }
    }
}

void GXCamera::run()
{
    GX_STATUS status = GXStreamOn(_devhandle);
    if(status != GX_STATUS_SUCCESS)
        throw_runtime_error("GXStreamOn failed ", get_cam_status_desc());
    _thread = std::thread(&GXCamera::main_loop, this);
}

void GXCamera::stop()
{
    _stop = true;
    if (_thread.joinable()) {
        _thread.join();
    }
}
