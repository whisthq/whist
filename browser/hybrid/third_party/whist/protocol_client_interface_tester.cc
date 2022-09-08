#include "base/logging.h"
#include "base/threading/platform_thread.h"
#include "protocol_client_interface.h"

int main(int argc, const char* argv[]) {
  if (!InitializeWhistClient()) {
    return 1;
  }
  base::PlatformThread::Sleep(base::Milliseconds(300));
  WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.disconnect);

  return 0;
}
