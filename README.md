# AutoAim for FPS Games

不要用于游戏，仅作技术尝试

### 使用方法
1. 修改配置 config.json
    ```python

    {
        "windowName" : "",                      # 目标窗口名称
        "pythonHome":"Python39/",                           # 本地python环境或者，release版本中自带的python环境
        "modelPath" : "models/yolov7-tiny-11w-320x320.trt", # 模型文件
        "mouseMovementDelay":0,                             # 鼠标移动事件与捕获下一帧屏幕之间的间隔 单位ms
    }
    ```
2. 修改 core.py
其中的 __process__ 方法会在一个AI帧调用，可以用 pyhton 自行编写鼠标移动逻辑。

    win32api.mouse_event(win32con.MOUSEEVENTF_MOVE, x, y, 0, 0) 操作需要**管理员权限**
    需要修改的参数：

    * FOV：真实 FOV，与游戏内FOV不一样 需要在这里查询
    * SENS: 游戏内灵敏度
    * ADS 每一个倍镜的灵敏度（不同倍镜的灵敏度是不一样的，没有实现切换倍镜的逻辑，默认按照 1 倍处理的但是影响不大，如果需要可以自行修改python脚本添加功能）

### 模型训练 
我参考的 [WongKinYiu/yolov7](https://github.com/WongKinYiu/yolov7)
```shell
# 可以自己训练模型然后 导出到 .rtr
wget https://github.com/WongKinYiu/yolov7/releases/download/v0.1/yolov7-tiny.pt
python export.py --weights ./yolov7-tiny.pt --grid --end2end --simplify --topk-all 100 --iou-thres 0.65 --conf-thres 0.35 --img-size 640 640
git clone https://github.com/Linaom1214/tensorrt-python.git
python ./tensorrt-python/export.py -o yolov7-tiny.onnx -e yolov7-tiny-nms.trt -p fp16
UI 选项
```

### UI选项
    Inference ： 是否开启推理（检测目标）
    Script ： 是否执行 core.py 中的 process 方法
    Preview ： 是否在游戏中标记出敌人
    命令行可以输入 script-reload 重新加载python脚本 方便调参数；

### 编译
cuda version 11.5 | TensorRT-8.2.5.1 | cudnn 11.3.58 | opencv 4.6 | 任意 python3
以 vs2022 为例：
1. opencv
    * 将<opencv_dir>\build\x64\vc15\bin 添加到**环境变量**
    * 将<opencv_dir>\build\include 添加到 **vs:属性->vc++目录->包含目录**
    * 将<opencv_dir>\build\x64\vc15\lib 添加到 **vs:属性->vc++目录->库目录**
2. cuda
    * 同opencv，将所需要的.dll 文件所在目录添加到**环境变量**
    * 同opencv，将所需要的.lib 文件所在目录添加到**库目录**
    * 同opencv，将所需要的.h 文件所在目录添加到**包含目录**
3. tensorrt
    同上
4. cudnn
    同上
5. python3
    同上

### 注意事项：
* 程序可能因为分配内存失败而崩溃，暂时不知道怎么解决
* 需要以管理员权限启动  (python: win32api.mouse_event 需要管理员权限才能生效)
* 关闭鼠标加速度       (windows系统和鼠标驱动都需要关闭加速度)
* 关闭windows缩放      (笔记本很多都是100%或150%)
* 以无边框窗口启动
* 如果 AI 帧率过低，可以降低**游戏分辨率**和**游戏最大帧率**
    以 RTX2060为例，1280p + 144fps情况下，AI帧率可以稳定在55-60fps
    如果不限制游戏帧率那么AI帧率只有20fps，AI帧率低于50就几乎跟不住枪了
    所以要么限制分辨率和游戏帧率，要么使用更好的显卡
