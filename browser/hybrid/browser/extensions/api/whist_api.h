// Copyright 2022 Whist Technologies, Inc. All rights reserved.
#ifndef WHIST_BROWSER_HYBRID_BROWSER_EXTENSIONS_API_WHIST_API_H_
#define WHIST_BROWSER_HYBRID_BROWSER_EXTENSIONS_API_WHIST_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class WhistBroadcastWhistMessageFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("whist.broadcastWhistMessage", UNKNOWN)

 protected:
  ~WhistBroadcastWhistMessageFunction() override {}

  ResponseAction Run() override;
};

class WhistGetKeyboardRepeatRateFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("whist.getKeyboardRepeatRate", UNKNOWN)

 protected:
  ~WhistGetKeyboardRepeatRateFunction() override {}

  ResponseAction Run() override;
};

class WhistGetKeyboardRepeatInitialDelayFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("whist.getKeyboardRepeatInitialDelay", UNKNOWN)

 protected:
  ~WhistGetKeyboardRepeatInitialDelayFunction() override {}

  ResponseAction Run() override;
};

class WhistGetSystemLanguageFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("whist.getSystemLanguage", UNKNOWN)

 protected:
  ~WhistGetSystemLanguageFunction() override {}

  ResponseAction Run() override;
};

class WhistGetUserLocaleFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("whist.getUserLocale", UNKNOWN)

 protected:
  ~WhistGetUserLocaleFunction() override {}

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // WHIST_BROWSER_HYBRID_BROWSER_EXTENSIONS_API_WHIST_API_H_
