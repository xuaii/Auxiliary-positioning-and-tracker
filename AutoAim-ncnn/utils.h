#pragma once
#include <codecvt>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <chrono>
#include <array>
#include <random>
#include <math.h>
#include <algorithm>
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX

#include "json.hpp"
using json = nlohmann::json;


#include <io.h>
#include <string>
#include <thread>

#include <vector>
#include <exception>
#include <cassert>

#include <Windows.h>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <Python.h>

#include <cuda_runtime_api.h>
#include "NvInfer.h"
#include "NvInferPlugin.h"
#include "NvInferRuntimeCommon.h"
#include "NvOnnxParser.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

#include <tchar.h>



using nvinfer1::Dims2;
using nvinfer1::Dims3;
using nvinfer1::IBuilder;
using nvinfer1::IBuilderConfig;
using nvinfer1::ICudaEngine;
using nvinfer1::IExecutionContext;
using nvinfer1::IHostMemory;
using nvinfer1::ILogger;
using nvinfer1::INetworkDefinition;
using Severity = nvinfer1::ILogger::Severity;

using cv::Mat;
using std::array;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::ofstream;
using std::string;
using std::vector;

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0) //必要的，我是背下来的 



class TargetBox;


struct ProgramState
{
    bool enableInference = false;
    bool enablePreview = false;
    bool enableScript = false;

    bool reloadScriptFlag = false;
    bool reloadModelFlag = false;

    int windowSize = 160;
    std::string modelPath = "";
    std::mutex lock;
    bool programRunning = true;
    std::vector<TargetBox> boxes;
    RECT window;
    cv::Size viewport;
    float fps = 0.0;
};
extern ProgramState state;


class TargetBox
{
private:
    float getWidth() { return (x2 - x1); };
    float getHeight() { return (y2 - y1); };

public:
    float x1;
    float y1;
    float x2;
    float y2;

    float ori_x1;
    float ori_y1;
    float ori_x2;
    float ori_y2;

    float area() { return getWidth() * getHeight(); };
};

struct Config
{
    std::string windowName = "";
    std::string pythonHome;
    std::string modelPath;
    int mouseMovementDelay;
};


namespace utils
{
    size_t vectorProduct(const std::vector<int64_t>& vector);
    std::wstring charToWstring(const char* str);

    template <typename T>
    T clip(const T& n, const T& lower, const T& upper);
    std::string fps(long deltaTime);
    LPCWSTR stringToLPCWSTR(std::string orig);
    class FpsRecoder
    {
    public:
        std::chrono::system_clock::time_point time;
        int count = 0;
        long long total_time = 00;
        FpsRecoder()
        {
            time = std::chrono::system_clock::now();
        }
        void update()
        {
            auto now = std::chrono::system_clock::now();
            total_time += std::chrono::duration_cast<std::chrono::microseconds>(now - time).count();
            time = now; count++;
            if (count % 1000 == 0)
            {
                count *= 0.05;
                total_time *= 0.05;
            }
        }
        float fps()
        {
            return 1000000.0 * (float)count / (float)total_time;
        }
    };
    static void draw_objects(cv::Mat& cvImg, const std::vector<TargetBox>& boxes)
    {
        for (size_t i = 0; i < boxes.size(); i++) {
            cv::rectangle(cvImg, cv::Point(boxes[i].ori_x1, boxes[i].ori_y1),
                cv::Point(boxes[i].ori_x2, boxes[i].ori_y2), cv::Scalar(255, 0, 0));
        }
    }
}

static void track(PyObject* pFunc, PyObject* dict, std::vector<TargetBox>& boxes)
{

    PyObject* list = PyList_New(0);
    Py_INCREF(list);

    for (auto& obj : boxes)
    {
        PyList_Append(list, Py_BuildValue("(f,f,f,f,f)", (obj.x1 + obj.x2) / 2, (obj.y1 + obj.y2) / 2, obj.x2 - obj.x1, obj.y2 - obj.y1, 1));
    }
    PyDict_SetItemString(dict, "target_list", list);
    Py_DECREF(list);
    PyDict_SetItemString(dict, "mouse_left_button", Py_BuildValue("b", KEY_DOWN(VK_LBUTTON)));
    PyDict_SetItemString(dict, "mouse_middle_button", Py_BuildValue("b", KEY_DOWN(VK_MBUTTON)));
    PyDict_SetItemString(dict, "mouse_right_button", Py_BuildValue("b", KEY_DOWN(VK_RBUTTON)));
    PyDict_SetItemString(dict, "mouse_ctrl_button", Py_BuildValue("b", KEY_DOWN(VK_CONTROL)));

    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, dict);

    PyObject* pRet = PyObject_CallObject(pFunc, args);
    if (!pRet) return;
    if (PyErr_Occurred())
    {
        PyErr_Print();
    }
    // 移动鼠标
}
