/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file index.tsx
 * @brief This file is the entry point of the renderer thread and acts as a router.
 */

import React, { useEffect, useState } from "react"
import ReactDOM from "react-dom"

import { OneButtonError, TwoButtonError } from "@app/renderer/pages/error"
import Signout from "@app/renderer/pages/signout"
import Importer from "@app/renderer/pages/importer"
import Update from "@app/renderer/pages/update"
import Launching from "@app/renderer/pages/launching"
import Importing from "@app/renderer/pages/importing"
import Omnibar from "@app/renderer/pages/omnibar"
import Background from "@app/renderer/pages/background"
import Welcome from "@app/renderer/pages/welcome"
import Support from "@app/renderer/pages/support"
import Onboarding from "@app/renderer/pages/onboarding"
import RestoreTabs from "@app/renderer/pages/restoreTabs"
import { Provider } from "@app/renderer/context/omnibar"

import {
  WindowHashSignout,
  WindowHashUpdate,
  WindowHashImport,
  WindowHashImportOnboarding,
  WindowHashOnboarding,
  WindowHashAuth,
  WindowHashLaunchLoading,
  WindowHashImportLoading,
  WindowHashOmnibar,
  WindowHashPayment,
  WindowHashSpeedtest,
  WindowHashLicense,
  WindowHashWelcome,
  WindowHashSupport,
  WindowHashRestoreTabs,
} from "@app/constants/windows"
import {
  whistError,
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  AUTH_ERROR,
  NAVIGATION_ERROR,
} from "@app/constants/error"
import { useMainState } from "@app/renderer/utils/ipc"
import { WhistTrigger } from "@app/constants/triggers"

// Electron has no way to pass data to a newly launched browser
// window. To avoid having to maintain multiple .html files for
// each kind of window, we pass a constant across to the renderer
// thread as a query parameter to determine which component
// should be rendered.

const RootComponent = () => {
  const [show, setShow] = useState(window.location.search.split("show=")[1])
  const [mainState, setMainState] = useMainState()

  const relaunch = () =>
    setMainState({
      trigger: { name: WhistTrigger.relaunchAction, payload: undefined },
    })

  const showPaymentWindow = () =>
    setMainState({
      trigger: { name: WhistTrigger.showPaymentWindow, payload: undefined },
    })

  const showSupportWindow = () =>
    setMainState({
      trigger: { name: WhistTrigger.showSupportWindow, payload: undefined },
    })

  const handleSignout = () =>
    setMainState({
      trigger: {
        name: WhistTrigger.clearCacheAction,
        payload: undefined,
      },
    })

  const showSignoutWindow = () =>
    setMainState({
      trigger: { name: WhistTrigger.showSignoutWindow, payload: undefined },
    })

  const getOtherBrowserWindows = (browser: string) =>
    setMainState({
      trigger: {
        name: WhistTrigger.getOtherBrowserWindows,
        payload: { browser },
      },
    })

  const handleImporterSubmit = (browser: string | undefined) => {
    console.log("beginning import for", browser)
    setMainState({
      trigger: {
        name: WhistTrigger.beginImport,
        payload: { importBrowserDataFrom: browser },
      },
    })
  }

  const handleWelcomeSubmit = () =>
    setMainState({
      trigger: {
        name: WhistTrigger.showAuthWindow,
        payload: undefined,
      },
    })

  const handleSupportClose = () =>
    setMainState({
      trigger: {
        name: WhistTrigger.closeSupportWindow,
        payload: undefined,
      },
    })

  const handleRestoreTabs = (urls: string[]) =>
    setMainState({
      trigger: {
        name: WhistTrigger.importTabs,
        payload: { urls },
      },
    })

  const handleOnboardingSubmit = () => {
    if (!["active", "trialing"].includes(mainState.subscriptionStatus)) {
      setMainState({
        trigger: {
          name: WhistTrigger.showPaymentWindow,
          payload: undefined,
        },
      })
    } else {
      setShow(WindowHashImportOnboarding)
    }
  }

  useEffect(() => {
    // We need to ask the main thread to re-emit the current StateIPC because
    // useMainState() only subscribes to state updates after the function is
    // called
    setMainState({
      trigger: { name: WhistTrigger.emitIPC, payload: undefined },
    })
  }, [])

  if (show === WindowHashOmnibar)
    return (
      <Provider>
        <Omnibar />
      </Provider>
    )
  if (show === WindowHashWelcome)
    return <Welcome onSubmit={handleWelcomeSubmit} />
  if (
    [
      WindowHashOmnibar,
      WindowHashAuth,
      WindowHashPayment,
      WindowHashSpeedtest,
      WindowHashLicense,
    ].includes(show)
  )
    return <Background />
  if (show === WindowHashSupport)
    return (
      <Support onClose={handleSupportClose} userEmail={mainState.userEmail} />
    )
  if (show === WindowHashSignout) return <Signout onClick={handleSignout} />
  if (show === WindowHashUpdate) return <Update />
  if (show === WindowHashImport)
    return (
      <Importer
        browsers={mainState.browsers ?? []}
        onSubmit={(browser: string | undefined) =>
          handleImporterSubmit(browser)
        }
        allowSkip={false}
        mode="importing"
      />
    )
  if (show === WindowHashImportOnboarding)
    return (
      <Importer
        browsers={mainState.browsers ?? []}
        onSubmit={(browser: string | undefined) =>
          handleImporterSubmit(browser)
        }
        allowSkip={true}
        mode="onboarding"
      />
    )
  if (show === WindowHashRestoreTabs)
    return (
      <RestoreTabs
        windows={mainState.otherBrowserWindows}
        browsers={mainState.browsers}
        onSubmitBrowser={(browser: string) => {
          getOtherBrowserWindows(browser)
        }}
        onSubmitWindow={(urls: string[]) => {
          handleRestoreTabs(urls)
        }}
      />
    )

  if (show === WindowHashOnboarding)
    return <Onboarding onSubmit={handleOnboardingSubmit} />
  if (show === WindowHashLaunchLoading)
    return <Launching networkInfo={mainState.networkInfo} />
  if (show === WindowHashImportLoading) return <Importing />
  if (show === NO_PAYMENT_ERROR)
    return (
      <TwoButtonError
        title={whistError[show].title}
        text={whistError[show].text}
        primaryButtonText="Update Payment"
        secondaryButtonText="Sign Out"
        onPrimaryClick={showPaymentWindow}
        onSecondaryClick={showSignoutWindow}
        onRequestHelp={showSupportWindow}
      />
    )
  if ([UNAUTHORIZED_ERROR, AUTH_ERROR, NAVIGATION_ERROR].includes(show))
    return (
      <OneButtonError
        title={whistError[show].title}
        text={whistError[show].text}
        primaryButtonText="Sign Out"
        onPrimaryClick={showSignoutWindow}
        onRequestHelp={showSupportWindow}
      />
    )
  if (Object.keys(whistError).includes(show))
    return (
      <OneButtonError
        title={whistError[show].title}
        text={whistError[show].text}
        primaryButtonText="Try Again"
        onPrimaryClick={relaunch}
        onRequestHelp={showSupportWindow}
      />
    )
  return (
    <OneButtonError
      title={whistError[NAVIGATION_ERROR].title}
      text={whistError[NAVIGATION_ERROR].text}
      primaryButtonText="Sign out"
      onPrimaryClick={showSignoutWindow}
      onRequestHelp={showSupportWindow}
    />
  )
}

// TODO: actually pass version number through IPC.
const WindowBackground = (props: any) => {
  return (
    <div className="relative w-full h-full bg-opacity-0 select-none">
      {props.children}
    </div>
  )
}

ReactDOM.render(
  <WindowBackground>
    <RootComponent />
  </WindowBackground>,
  document.getElementById("root")
)
