// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "protocol_client_interface.h"

#include <string.h>
#include <thread>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/native_library.h"
#include "base/path_service.h"

typedef const WhistClient::VirtualInterface* (*VirtualInterfaceCreator)(void);
const WhistClient::VirtualInterface* whist_virtual_interface = NULL;
static base::Lock whist_virtual_interface_lock;

// These must be static; can't be on the stack since the protocol thread will always expect them
// to be around.
static const char* protocol_argv[] = {"libWhistClient", "--frontend=virtual", "--dynamic-arguments"};
static int protocol_argc = sizeof(protocol_argv) / sizeof(protocol_argv[0]);

// This implementation is inspired gl_initializer_*.cc and gl_implementation.cc
static base::NativeLibrary LoadWhistClientLibrary() {
  // First, compute the path to the library.
  base::FilePath lib_dir_path;
#if BUILDFLAG(IS_MAC)
#define LIB_SUFFIX "dylib"
  if (base::mac::AmIBundled()) {
    lib_dir_path = base::mac::FrameworkBundlePath().Append("Libraries");
  } else {
    if (!base::PathService::Get(base::FILE_EXE, &lib_dir_path)) {
      LOG(ERROR) << "PathService::Get failed.";
      return NULL;
    }
    lib_dir_path = lib_dir_path.DirName();
  }
#elif BUILDFLAG(IS_WIN)
#define LIB_SUFFIX "dll"
  if (!base::PathService::Get(base::DIR_MODULE, &base_dir)) {
    LOG(ERROR) << "PathService::Get failed.";
    return NULL;
  }
#else
#define LIB_SUFFIX "so"
  if (!base::PathService::Get(base::FILE_EXE, &lib_dir_path)) {
    LOG(ERROR) << "PathService::Get failed.";
    return NULL;
  }
  lib_dir_path = lib_dir_path.DirName();
#endif

  base::FilePath lib_whist_client_path = lib_dir_path.Append("libWhistClient." LIB_SUFFIX);

  // Using the computed path, attempt to load the library.
  base::NativeLibraryLoadError error;
  base::NativeLibrary library = base::LoadNativeLibrary(lib_whist_client_path, &error);
  if (!library) {
    LOG(ERROR) << "Failed to load " << lib_whist_client_path.MaybeAsASCII() << ": "
               << error.ToString();
    return NULL;
  }
  return library;
}

void InitializeWhistClient() {
  base::AutoLock whist_client_auto_lock(whist_virtual_interface_lock);

  if (whist_virtual_interface != NULL) {
    return;
  }

  base::NativeLibrary whist_client_library = LoadWhistClientLibrary();
  if (!whist_client_library) {
    return;
  }

  VirtualInterfaceCreator get_virtual_interface =
      reinterpret_cast<VirtualInterfaceCreator>(
          base::GetFunctionPointerFromNativeLibrary(whist_client_library,
                                                    "get_virtual_interface"));

  if (get_virtual_interface) {
    whist_virtual_interface = get_virtual_interface();
  }

  // Initialize whist, so that connections can be made from javascript later
  WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.initialize, protocol_argc, protocol_argv);
  // TODO: lifecycle.destroy sometime? If necessary?

  // Initialize logging
  int session_id = WHIST_VIRTUAL_INTERFACE_CALL(logging.get_session_id);

  WHIST_VIRTUAL_INTERFACE_CALL(logging.set_callback, [session_id](unsigned int level, const char* line) {
    LOG(ERROR) << "SESSION ID IS: " << session_id;
  });
}
