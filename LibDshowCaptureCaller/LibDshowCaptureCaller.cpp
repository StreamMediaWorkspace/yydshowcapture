// LibDshowCaptureCaller.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <objbase.h>
#include "../dshowcapture.hpp"
#include "./libyuv.h"


using namespace DShow;

#pragma comment(lib, "../vs/2015/Debug/dshowcapture.lib")
#pragma comment(lib, "./libs/release/turbojpeg-static.lib")
#ifdef _DEBUG
#pragma comment(lib, "./libs/debug/yuv.lib")
#else
#pragma comment(lib, "./libs/release/yuv.lib")
#endif // DEBUG



void Log(LogType type, const wchar_t *msg, void *param) {
    switch (type) {
    case LogType::Error:
        printf("[Error]");
        break;
    case LogType::Debug:
        printf("[Debug]");
        break;
    case LogType::Info:
        printf("[Info]");
        break;
    case LogType::Warning:
        printf("[Warning]");
        break;
    default:
        printf("[None]");
        break;
    }
    wprintf(L"%s\n", msg);
}

void OnAudioData(const AudioConfig &config, unsigned char *data,
    size_t size, long long startTime, long long stopTime) {
    printf("audio data format(%d)\n", config.format);
}

#define DEBUG_YUV_DATA 1
void OnVideoData(const VideoConfig &config, unsigned char *data,
    size_t size, long long startTime, long long stopTime) {
    printf("video data format(%d)\n", config.format);
#ifdef DEBUG_YUV_DATA
    static FILE *srcYuvFile = nullptr;
    if (!srcYuvFile) {
        char path[512] = { 0 };
        sprintf_s(path, sizeof(path), "./src_test_%dX%d.yuv", config.cx, config.cy_abs);
        fopen_s(&srcYuvFile, path, "wb+");
    }
    if (srcYuvFile) {
        fwrite(data, size, 1, srcYuvFile);
    }

    static FILE *yuvFile = nullptr;
    if (!yuvFile) {
        char path[512] = { 0 };
        snprintf(path, sizeof(path), "./test_%dX%d.yuv", config.cx, config.cy_abs);
        fopen_s(&yuvFile, path, "wb+");
    }
#endif
    const size_t size_required = config.cx * config.cy_abs * 3 / 2;
    static size_t yuv_size = 0;
    static uint8_t *yuv420 = nullptr;
    if (yuv_size != size) {
        yuv_size = size;
        if (yuv420) {
            delete[] yuv420;
        }
        yuv420 = new uint8_t[size_required];
    }
    
    int ret = -1;
    libyuv::FourCC forcc = libyuv::FourCC::FOURCC_ANY;
    switch (config.format) {
    case VideoFormat::YUY2:
        forcc = libyuv::FourCC::FOURCC_YUY2;
        break;

    case VideoFormat::MJPEG:
        forcc = libyuv::FourCC::FOURCC_MJPG;
        break;
    }

    ret = libyuv::ConvertToI420(data, size,
        yuv420, config.cx,
        yuv420 + config.cx * config.cy_abs, config.cx / 2,
        yuv420 + config.cx * config.cy_abs * 5 / 4, config.cx / 2,
        0, 0, config.cx, config.cy_abs, config.cx, config.cy_abs,
        libyuv::RotationMode::kRotate0, forcc);

#ifdef DEBUG_YUV_DATA
    if (!ret && yuv420 && yuvFile) {
        fwrite(yuv420, size_required, 1, yuvFile);
    }
#endif
}

int main()
{
    CoInitialize(NULL);
    do {
        SetLogCallback(Log, nullptr);

        Device device(InitGraph::True);
        
        std::vector<AudioDevice> audioDevices;
        Device::EnumAudioDevices(audioDevices);

        std::vector<VideoDevice> videoDevices;
        Device::EnumVideoDevices(videoDevices);

        AudioConfig audioConfig;
        audioConfig.channels = 2;
        audioConfig.format = AudioFormat::AAC;
        audioConfig.sampleRate = 441000;
        audioConfig.callback = OnAudioData;
        audioConfig.name = audioDevices.at(0).name;
        if (!device.SetAudioConfig(&audioConfig)) {
            ERROR("SetAudioConfig failed");
            break;
        }

        VideoConfig videoConfig;
        videoConfig.format = VideoFormat::MJPEG;
        videoConfig.cx = 1080;
        videoConfig.cy_abs = 720;
        videoConfig.name = videoDevices.at(1).name;
        videoConfig.callback = OnVideoData;
        if (!device.SetVideoConfig(&videoConfig)) {
            ERROR("SetVideoConfig failed");
            break;
        }

        if (!device.ConnectFilters()) {
            ERROR("ConnectFilters failed\n");
            break;
        }
        device.Start();
        while (1) {
            Sleep(10);
        }

        device.Stop();
    } while (0);

    system("pause");
    CoUninitialize();
    return 0;
}

