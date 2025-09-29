
#include "newE.h"

#include <windows.h>

#include <Python.h>
#include <psapi.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "newE_src/newE_python.mojom.h"

base::NoDestructor<Python_Data> g_python_data;
const bool NewE_Converter::SYNC_WORK_MODE = true;  // 目前先测试同步模式
base::NoDestructor<mojo::Remote<convert_to_new_e_by_python::ConvertToNewEUI>>
    NewE_Converter::g_newE_remote;  // 2025.07.04

void bind_newE_Python_service(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<convert_to_new_e_by_python::ConvertToNewEUI>
        receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<NewE_Converter>(std::move(receiver)),
      std::move(receiver));
}

// 获取命令行参数字符串
std::wstring GetCommandLineString() {
  return GetCommandLineW();
}

// 获取当前的进程名（exe文件名）
std::string GetProcessNameByPid(void) {
  const DWORD pid = GetCurrentProcessId();  // 获取当前进程ID
  LOG(ERROR) << "GetCurrentProcessId() == " << pid;

  HANDLE hProcess =
      OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (!hProcess) {
    return "";
  }

  char processName[MAX_PATH] = "<unknown>";
  if (GetModuleBaseNameA(hProcess, NULL, processName,
                         sizeof(processName) / sizeof(char))) {
    CloseHandle(hProcess);
    return processName;
  }
  CloseHandle(hProcess);
  return "";
}

bool init_all_of_newE(void) {
  // base::CommandLine::Init(0, nullptr);  // 或用实际参数
  base::CommandLine::Init(__argc, __argv);

  // 自动添加 --no-sandbox 参数，确保进程不启用沙箱，2025.07.06
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch("no-sandbox")) {
    command_line->AppendSwitch("no-sandbox");   //经过多次确认，Chrome + newE时，也必须去沙箱，2025.07.19
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch("type")) {
    return false;  // 子进程不初始化, 2025.07.08
  }

  // 主进程才初始化, 2025.07.08
  return g_python_data->init_all_Data();
}

//*********************************************************** Class Python_Data
// Start
Python_Data::~Python_Data(void) {
  release_Data();
}

Python_Data::Python_Data(void) {
  g_PYTHON_Module = nullptr;
  g_PYTHON_Func = nullptr;
  g_identifier = nullptr;
}

void Python_Data::release_Data(void) {
  // 这是个静态函数，供以后在外边回收内存用的
  if (g_PYTHON_Func) {
    Py_XDECREF(g_PYTHON_Func);
    g_PYTHON_Func = nullptr;  // 清空函数指针
  }

  if (g_PYTHON_Module) {
    Py_XDECREF(g_PYTHON_Module);
    g_PYTHON_Module = nullptr;  // 清空模块指针
  }

  if (Py_IsInitialized()) {
    Py_Finalize();
  }

  if (g_identifier) {
    g_identifier.reset();
    g_identifier = nullptr;  // 清空语言识别器指针
  }
}

chrome_lang_id::NNetLanguageIdentifier* Python_Data::set_g_identifier(
    const size_t asked_size) {
  static size_t old_size = 512;

  if (g_identifier) {
    if (asked_size > old_size) {
      g_identifier = std::make_unique<chrome_lang_id::NNetLanguageIdentifier>(
          0, asked_size + 1);
      old_size = asked_size;
    }
  } else {
    g_identifier = std::make_unique<chrome_lang_id::NNetLanguageIdentifier>(
        0, old_size + 1);
  }

  return g_identifier.get();
}

bool Python_Data::init_PYTHON(void) {
  if (g_PYTHON_Func) {
    return true;  // 如果函数已经初始化，直接返回 true
  }

  wchar_t exePath[MAX_PATH] = {0};
  GetModuleFileNameW(NULL, exePath, MAX_PATH);
  std::wstring newEExeDir(exePath);  // 待求的本exe所在目录(绝对路径)
  size_t pos = newEExeDir.find_last_of(L"\\/");
  if (pos != std::wstring::npos) {
    newEExeDir = newEExeDir.substr(0, pos);
  }

  // 下面是假设本exe所在文件夹下，有一个python_embed文件夹，
  std::wstring pythonHome = newEExeDir + L"\\python_embed";
  Py_SetPythonHome(const_cast<wchar_t*>(pythonHome.c_str()));
  // SetDllDirectoryW(pythonHome.c_str());
  // 这句风险太大(导致所有的dll都到那个文件夹找)，所以要屏蔽！
  // 直接复制python313.dll到exe所在目录下，最简洁

  // 下面代码在旧电脑上测试通过了的版本
  if (!Py_IsInitialized()) {
    Py_Initialize();
    // PyEval_InitThreads();  // Python 3.7 及以上版本，PyEval_InitThreads()
    // 已经不需要手动调用，但安全见。。。
    if (!Py_IsInitialized()) {
      LOG(ERROR) << "Python初始化失败，请检查Python环境是否正确安装？";
      return false;
    }
  }

  // 添加exe所在(绝对路径)目录到sys.path，确保可以以包名方式导入newE_pt.newE
  // LOG(ERROR) << "newEExeDir == " << std::wstring(newEExeDir);
  const int len = WideCharToMultiByte(CP_UTF8, 0, newEExeDir.c_str(), -1, NULL,
                                      0, NULL, NULL);
  std::string newEExeDirUtf8(len, 0);
  WideCharToMultiByte(CP_UTF8, 0, newEExeDir.c_str(), -1, &newEExeDirUtf8[0],
                      len, NULL, NULL);
  if (!newEExeDirUtf8.empty() && newEExeDirUtf8.back() == '\0') {
    newEExeDirUtf8.pop_back();
  }
  std::string cmd = "import sys; sys.path.insert(0, r'" + newEExeDirUtf8 + "')";
  PyRun_SimpleString(cmd.c_str());
  // PyRun_SimpleString("import sys,os; print('sys.path:', sys.path);
  // print('cwd:', os.getcwd())"); LOG(ERROR) <<
  // "当前exe执行程序的目录已经写到python可以搜索得到的路径";

  if (!g_PYTHON_Module) {
    // 用包名方式导入
    g_PYTHON_Module = PyImport_ImportModule("newE_py.newE");
    if (!g_PYTHON_Module) {
      // PyErr_Print();
      LOG(ERROR) << "并没有找到newE.py文件(、或newE_py文件夹不存在)?";
      return false;
    }
  }

  // 获取 Python 模块中的函数
  g_PYTHON_Func = PyObject_GetAttrString(g_PYTHON_Module, "newE_by_spaCy");
  if (g_PYTHON_Func) {
    if (!PyCallable_Check(g_PYTHON_Func)) {
      LOG(ERROR) << "newE.py并没有newE_by_spaCy函数?";
      Py_XDECREF(g_PYTHON_Func);
      g_PYTHON_Func = nullptr;  // 如果不是可调用对象，清空函数指针

      return false;
    } else {
      // LOG(ERROR) << "python已经成功地加载了关键的转换函数newE_by_spaCy!";
      return true;
    }
  } else {
    // LOG(ERROR) << "newE.py并没有newE_by_spaCy函数?";
    return false;  // 如果获取函数失败，直接返回 false;
  }
}

bool Python_Data::self_test(void) {
  static bool self_test_done = false;
  if (self_test_done) {
    return !selfTestResult.empty();
  }

  PythonThreadId = std::this_thread::get_id();
  self_test_done = true;

  return callPython("I taught him Chinese many years ago",
                    selfTestResult);  // 测试用的字符串
}

bool Python_Data::init_all_Data(void) {
  if (!set_g_identifier(0)) {
    release_Data();
    return false;  // 如果获取语言识别器失败，直接返回 false
  }

  if (!init_PYTHON())  // 初始化 Python 环境并加载函数
  {
    release_Data();
    return false;
  }

  if (!self_test())  // 执行自检
  {
    release_Data();
    return false;  // 如果自检失败，直接返回 false
  }

  return true;
}

const std::string& Python_Data::get_selfTestResult(void) const {
  return selfTestResult;
}

bool Python_Data::is_PythonThreadId_NULL(void) const {
  return PythonThreadId == std::thread::id();
}

bool Python_Data::callPython(const std::string& o_S, std::string& newE_S) {
  newE_S.clear();  // 保证每次调用都清空

  // Chromium开发环境里，要调用Python函数，必须在主线程里调用，
  // 另外，初始化Python的线程和调用callPython的线程必须相同，否则会导致崩溃(或进入死机状)。

  if (PythonThreadId != std::this_thread::get_id()) {
    LOG(ERROR) << "The current thread is not equale to PythonThreadId!";
    return false;  // 如果不是在Python线程里调用，直接返回 false;
  } else {
    // LOG(ERROR) << "The current thread is safe to callPython...";
  }

  if (!g_PYTHON_Func) {
    return false;  // 如果函数未初始化，直接返回 false;
  }

  chrome_lang_id::NNetLanguageIdentifier* identifier =
      set_g_identifier(o_S.size());
  if (!identifier) {
    return false;  // 如果函数未初始化，直接返回 false;
  }

  auto result = identifier->FindLanguage(o_S);
  if (result.language != "en") {
    // LOG(ERROR) << "not English...";
    return false;
  }

  PyGILState_STATE gstate = PyGILState_Ensure();  // 获取GIL(独家使用权)
  PyObject* pArgs = PyTuple_Pack(1, PyUnicode_FromString(o_S.c_str()));

  if (!pArgs) {
    LOG(ERROR) << "无法创建参数元组?";
    PyGILState_Release(gstate);  // 释放GIL
    return false;                // 如果参数创建失败，直接返回 false;
  }

  PyObject* pValue = PyObject_CallObject(g_PYTHON_Func, pArgs);

  if (pValue) {
    if (PyUnicode_Check(pValue)) {
      // newE_S = PyUnicode_AsUTF8(pValue);  // 获取 Python 函数返回的字符串
      const char* pyResult =
          PyUnicode_AsUTF8(pValue);  // 目的是检查指针的安全性
      if (pyResult) {
        newE_S = pyResult;
      }
    }

    Py_DECREF(pValue);
  }

  Py_DECREF(pArgs);
  PyGILState_Release(gstate);  // 释放GIL
  // 判断字符串长度
  return !newE_S.empty();
}
//************************************************************* Class
// Python_Data End

//******************************************************** Class NewE_Converter
// Start
NewE_Converter::NewE_Converter(
    mojo::PendingReceiver<convert_to_new_e_by_python::ConvertToNewEUI> receiver)
    : receiver_(this, std::move(receiver)) {
  //LOG(ERROR) << "NewE_Converter constructed";
}

NewE_Converter::~NewE_Converter() {
  //LOG(ERROR) << "NewE_Converter destructed";
}

void NewE_Converter::work(const std::string& o_S, workCallback callback) {
  static bool logged_selfTestResult = false;
  if (!logged_selfTestResult) {
    LOG(ERROR) << "Async_work--->selfTestResult: "
               << g_python_data->get_selfTestResult();
    logged_selfTestResult = true;  // 只记录一次
  }

  std::string result;
  if (g_python_data->is_PythonThreadId_NULL()) {
    LOG(ERROR) << "Async_work: PythonThreadId == NULL??????";
    // result = "Hello, world... from NewE_Converter::Async_work, 2025.07.09";
    result = "";
  } else {
    g_python_data->callPython(o_S, result);
  }

  std::move(callback).Run(result);
  // 经测试，发现本函数自身没有问题，但回到浏览器那端就有问题，
  // 所以，现在还是坚持以前的看法，暂时先用同步模式来处理。
}

void NewE_Converter::work_sync(const std::string& script,
                                work_syncCallback callback) {
  static bool logged_selfTestResult = false;
  if (!logged_selfTestResult) {
    LOG(ERROR) << "SYNC_work--->selfTestResult: "
               << g_python_data->get_selfTestResult();
    logged_selfTestResult = true;  // 只记录一次
  }

  std::string result;
  if (g_python_data->is_PythonThreadId_NULL()) {
    LOG(ERROR) << "SYNC_work: PythonThreadId == NULL??????";
    // result = "Hello, world... from NewE_Converter::SYNC_work, 2025.07.09";
    result = "";
  } else {
    g_python_data->callPython(script, result);
  }

  std::move(callback).Run(result);
}

bool NewE_Converter::work_sync(const std::string& o_S, std::string* n_S) {
  //LOG(ERROR) << "work_sync (bool) called";
  *n_S = "";
  return true;
}
//********************************************************** Class
// NewE_Converter End

bool only_for_newE_test(const std::string& o_S, std::string& n_S) {
  // 2025.07.10
  n_S = "only_for_newE_test!";
  return true;
}