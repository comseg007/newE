#ifndef NEW_E_H_
#define NEW_E_H_

#include <thread>

#include "base/memory/raw_ptr.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "newE_src/newE_python.mojom.h"
#include "third_party/cld_3/src/src/nnet_language_identifier.h"

// #pragma once	//Copilot建议要屏蔽的

class Python_Data {
 private:
  std::thread::id PythonThreadId;  // 用于记录Python线程ID
  raw_ptr<struct _object> g_PYTHON_Module;
  raw_ptr<struct _object> g_PYTHON_Func;
  std::unique_ptr<chrome_lang_id::NNetLanguageIdentifier> g_identifier;
  std::string selfTestResult;

  void release_Data(void);
  bool self_test(void);
  chrome_lang_id::NNetLanguageIdentifier* set_g_identifier(
      const size_t asked_size);
  bool init_PYTHON(void);

 public:
  ~Python_Data(void);
  Python_Data(void);

  bool callPython(const std::string& o_S, std::string& newE_S);
  bool init_all_Data(void);

  const std::string& get_selfTestResult(void) const;
  bool is_PythonThreadId_NULL(void) const;
};

// 经受了较长时间地无法把newE注册到Chrome的烦恼、折磨后，
// 现(经与Coplit协作)、确认：必须严格按照下面的格式(样式、顺序？)来设置mojom、IPC的！
// 否则很可能又注册不了！2025.07.19
class NewE_Converter : public convert_to_new_e_by_python::ConvertToNewEUI {
 public:
  explicit NewE_Converter(
      mojo::PendingReceiver<convert_to_new_e_by_python::ConvertToNewEUI>
          receiver);
  ~NewE_Converter() override;   //不能放在构造函数之前？

  void work(const std::string& o_S, workCallback callback) override;
  void work_sync(const std::string& script,
                 work_syncCallback callback) override;
  bool work_sync(const std::string& o_S, std::string* n_S) override;

  static const bool SYNC_WORK_MODE;  // 是否同步工作
  static base::NoDestructor<
      mojo::Remote<convert_to_new_e_by_python::ConvertToNewEUI>>
      g_newE_remote;  // 2025.07.04

 private:
  mojo::Receiver<convert_to_new_e_by_python::ConvertToNewEUI> receiver_;
};

bool init_all_of_newE(void);

void bind_newE_Python_service(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<convert_to_new_e_by_python::ConvertToNewEUI>
        receiver);

bool only_for_newE_test(const std::string& o_S,
                        std::string& n_S);  // 2025.07.10

#endif  // NEW_E_H_
