#include "inference.h"
#include "yolov7.h"
void inference_thread_func()
{
	CustomConsole::Instance().AddLog("# init config ...");
	// ----------------------- config -----------------------
	std::ifstream f = std::ifstream();
	json json_data;;

	Config config;
	// 需要根据当前运行环境构造文件全名
	try {
		f.open("./config/config.json");
		json_data = json::parse(f);
		CustomConsole::Instance().AddLog("# load config from: config.json");
		config.windowName = json_data["windowName"];
		config.pythonHome = json_data["pythonHome"];
		config.mouseMovementDelay = json_data["mouseMovementDelay"];
		config.modelPath = json_data["modelPath"];
	}
	catch (std::exception& e) {
		CustomConsole::Instance().AddLog("load config file faild");
		return;
	}

	// ----------------------- grabber -----------------------
	CustomConsole::Instance().AddLog("# init screen grabber ...");

	// 要求是无边框窗口
	HWND handle; RECT rect;
	handle = FindWindow(NULL, utils::stringToLPCWSTR(config.windowName));
	if (!handle)
	{
		CustomConsole::Instance().AddLog("cant find window");
		return;
	}
	GetWindowRect(handle, &rect);
	grabber_t* grabber;
	grabber_crop_area_t crop;
	crop.x = 0; crop.y = 0; crop.width = 0; crop.height = 0;
	grabber = grabber_create(handle, crop, 3);

	// ----------------------- TensorRT --------------------------
	CustomConsole::Instance().AddLog("# init TensorRT framework ...");
	CustomConsole::Instance().AddLog("load model from:%s", config.modelPath);
	char* pc = new char[200];
	strcpy(pc, config.modelPath.c_str());
	Yolo yolo(pc);

	// ----------------------- python --------------------------
	CustomConsole::Instance().AddLog("# init python runtime ...");
	wchar_t* program = Py_DecodeLocale(".", NULL);
	if (program == NULL) {
		CustomConsole::Instance().AddLog("Fatal error: cannot decode argv[0]");
		exit(1);
	}
	
	Py_SetProgramName(program);
	Py_SetPythonHome(std::wstring(config.pythonHome.begin(), config.pythonHome.end()).c_str());
	Py_Initialize();
	if (!Py_IsInitialized()) {
		CustomConsole::Instance().AddLog("python init fail");
		return;
	}
	PyRun_SimpleString("# python runtime inited");
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('.')");
	PyObject* dict = PyDict_New();
	Py_INCREF(dict);

	PyDict_SetItemString(dict, "window_rect", Py_BuildValue("{s:i,s:i,s:i,s:i}", "left", static_cast<int>(rect.left), "bottom", static_cast<int>(rect.bottom), "right", static_cast<int>(rect.right), "top", static_cast<int>(rect.top)));
	
	PyObject* pmodel = PyImport_ImportModule("core");
	if (PyErr_Occurred()) PyErr_Print();
	if (!pmodel)
	{
		CustomConsole::Instance().AddLog("! Can't find \'core\' module");
	}
	Py_INCREF(pmodel);
	PyObject* pFunc = PyObject_GetAttrString(pmodel, "__process__");
	if (!pFunc)
	{
		CustomConsole::Instance().AddLog("! Can't find \'__process__\' method");
	}
	Py_INCREF(pFunc);
	// ----------------------- main loop -----------------------
	CustomConsole::Instance().AddLog("# inference module init complete...");

	cv::Mat frame; cv::Mat croped;

	utils::FpsRecoder recoder;
	while (state.programRunning)
	{
		std::vector<TargetBox> boxes;

		// caputer
		GetWindowRect(handle, &rect);
		void* pixel = grabber_grab(grabber);
		frame = cv::Mat(cv::Size{ grabber->width, grabber->height }, CV_8UC3);
		frame.data = (uchar*)pixel;
		// 处理重新加载脚本等事件
		if (state.reloadScriptFlag)
		{
			pmodel = PyImport_ReloadModule(pmodel);
			if (PyErr_Occurred()) PyErr_Print();
			if (!pmodel)
			{
				CustomConsole::Instance().AddLog("! Can't find \'core\' module");
			}
			Py_INCREF(pmodel);
			Py_DECREF(pFunc);
			pFunc = PyObject_GetAttrString(pmodel, "__process__");
			if (!pFunc)
			{
				CustomConsole::Instance().AddLog("! Can't find \'__process__\' method");
			}
			Py_INCREF(pFunc);
			state.reloadScriptFlag = false;
			CustomConsole::Instance().AddLog("# reload success");
		}
		// detect
		if (state.enableInference)
		{
			yolo.Infer(frame, boxes, croped);
			// track
			if (state.enableScript)
			{
				track(pFunc, dict, boxes);
			}
			if (state.enablePreview)
			{
				utils::draw_objects(croped, boxes);
				state.lock.lock();
				state.boxes.swap(boxes);
				state.viewport.width = croped.cols;
				state.viewport.height = croped.rows;
				state.window = rect;
				state.lock.unlock();
			}
		}

		cv::waitKey(config.mouseMovementDelay);
		recoder.update();
		state.fps = recoder.fps();
	}
	grabber_destroy(grabber);
	if (Py_FinalizeEx() < 0) {
		exit(0);
	}
	Py_DECREF(dict);
	Py_DECREF(pFunc);
	Py_DECREF(pmodel);
	PyMem_RawFree(program);
	CustomConsole::Instance().AddLog("Bye Hacker!");
}


