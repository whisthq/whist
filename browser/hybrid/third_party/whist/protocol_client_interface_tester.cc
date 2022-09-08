#include "base/logging.h"
#include "base/threading/platform_thread.h"
#include "protocol_client_interface.h"

int main(int argc, const char* argv[]) {
  InitializeWhistClient();
  base::PlatformThread::Sleep(base::Seconds(3));
  WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.disconnect);

  return 0;
}
