// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "protocol_client_interface.h"

#include <string.h>
#include <thread>
#include <vector>
#include <fstream>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#if BUILDFLAG(IS_MAC)
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#endif

// Defines for Whist shared lib name
#if BUILDFLAG(IS_MAC)
#define LIB_FILENAME_UTF8 "libWhistClient.dylib"
#define LIB_FILENAME "libWhistClient.dylib"
#elif BUILDFLAG(IS_WIN)
#define LIB_FILENAME_UTF8 "WhistClient.dll"
#define LIB_FILENAME L"WhistClient.dll"
#else
#define LIB_FILENAME_UTF8 "libWhistClient.so"
#define LIB_FILENAME "libWhistClient.so"
#endif

typedef const WhistClient::VirtualInterface* (*VirtualInterfaceCreator)(void);
const WhistClient::VirtualInterface* whist_virtual_interface = NULL;
static base::Lock whist_virtual_interface_lock;

// These must be static; can't be on the stack since the protocol thread will always expect them
// to be around.
static const char* protocol_argv[] = {LIB_FILENAME_UTF8, "--frontend=virtual", "--dynamic-arguments", NULL};
static int protocol_argc = sizeof(protocol_argv) / sizeof(protocol_argv[0]) - 1;

// This implementation is inspired gl_initializer_*.cc and gl_implementation.cc
static base::NativeLibrary LoadWhistClientLibrary() {
  // First, compute the path to the library.
  base::FilePath lib_dir_path;
#if BUILDFLAG(IS_MAC)
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
  if (!base::PathService::Get(base::DIR_MODULE, &lib_dir_path)) {
    LOG(ERROR) << "PathService::Get failed.";
    return NULL;
  }
#else
  if (!base::PathService::Get(base::FILE_EXE, &lib_dir_path)) {
    LOG(ERROR) << "PathService::Get failed.";
    return NULL;
  }
  lib_dir_path = lib_dir_path.DirName();
#endif

  base::FilePath lib_whist_client_path = lib_dir_path.Append(LIB_FILENAME);

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
static std::ofstream whist_logs_out;

bool InitializeWhistClient() {
  base::AutoLock whist_client_auto_lock(whist_virtual_interface_lock);

  if (whist_virtual_interface != NULL) {
    return true;
  }

  base::NativeLibrary whist_client_library = LoadWhistClientLibrary();
  if (!whist_client_library) {
    return false;
  }

  VirtualInterfaceCreator get_virtual_interface_ptr =
      reinterpret_cast<VirtualInterfaceCreator>(
          base::GetFunctionPointerFromNativeLibrary(whist_client_library,
                                                    "get_virtual_interface"));

  if (get_virtual_interface_ptr == NULL) {
    LOG(ERROR) << "Got value of NULL, or could not find, symbol get_virtual_interface";
    return true;
  } else {
    whist_virtual_interface = get_virtual_interface_ptr();
  }

  // Initialize whist, so that connections can be made from javascript later
  WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.initialize, protocol_argc, protocol_argv);
  // TODO: lifecycle.destroy sometime? If necessary?

  // Pipe protocol logs to a .log file
  base::FilePath path;
  base::PathService::Get(chrome::DIR_USER_DATA, &path);
  path = path.Append("whist_protocol_client.log");
  whist_logs_out.open(path.AsUTF8Unsafe().c_str());

  WHIST_VIRTUAL_INTERFACE_CALL(logging.set_callback, [](unsigned int level, const char* line) {
    whist_logs_out << line;
  });

  return true
}
