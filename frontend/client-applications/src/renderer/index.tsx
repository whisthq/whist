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
import Network from "@app/renderer/pages/network"
import Loading from "@app/renderer/pages/loading"
import Omnibar from "@app/renderer/pages/omnibar"
import Background from "@app/renderer/pages/background"
import Welcome from "@app/renderer/pages/welcome"
import Support from "@app/renderer/pages/support"
import { Provider } from "@app/renderer/context/omnibar"

import {
  WindowHashSignout,
  WindowHashUpdate,
  WindowHashImport,
  WindowHashOnboarding,
  WindowHashAuth,
  WindowHashLoading,
  WindowHashOmnibar,
  WindowHashPayment,
  WindowHashSpeedtest,
  WindowHashLicense,
  WindowHashWelcome,
  WindowHashSupport,
} from "@app/constants/windows"
import {
  whistError,
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  AUTH_ERROR,
  NAVIGATION_ERROR,
  LOCATION_CHANGED_ERROR,
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

  const handleImporterSubmit = (browser: string | undefined) => {
    setMainState({
      trigger: {
        name: WhistTrigger.beginImport,
        payload: { importBrowserDataFrom: browser },
      },
    })
  }

  const handleNetworkSubmit = () => setShow(WindowHashImport)

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
      />
    )
  if (show === WindowHashOnboarding) {
    return (
      <Network
        networkInfo={mainState.networkInfo}
        onSubmit={handleNetworkSubmit}
      />
    )
  }
  if (show === WindowHashLoading) {
    return <Loading networkInfo={mainState.networkInfo} />
  }
  if (show === NO_PAYMENT_ERROR)
    return (
      <TwoButtonError
        title={whistError[show].title}
        text={whistError[show].text}
        primaryButtonText="Update Payment"
        secondaryButtonText="Sign Out"
        onPrimaryClick={showPaymentWindow}
        onSecondaryClick={showSignoutWindow}
      />
    )
  if (
    [
      UNAUTHORIZED_ERROR,
      AUTH_ERROR,
      NAVIGATION_ERROR,
      LOCATION_CHANGED_ERROR,
    ].includes(show)
  )
    return (
      <OneButtonError
        title={whistError[show].title}
        text={whistError[show].text}
        primaryButtonText="Sign Out"
        onPrimaryClick={showSignoutWindow}
      />
    )
  if (Object.keys(whistError).includes(show))
    return (
      <TwoButtonError
        title={whistError[show].title}
        text={whistError[show].text}
        primaryButtonText="Try Again"
        secondaryButtonText="Sign Out"
        onPrimaryClick={relaunch}
        onSecondaryClick={showSignoutWindow}
      />
    )
  return (
    <TwoButtonError
      title={whistError.NAVIGATION_ERROR.title}
      text={whistError.NAVIGATION_ERROR.text}
      primaryButtonText="Try Again"
      secondaryButtonText="Sign Out"
      onPrimaryClick={relaunch}
      onSecondaryClick={showSignoutWindow}
    />
  )
}

// TODO: actually pass version number through IPC.
const WindowBackground = (props: any) => {
  return (
    <div className="relative w-full h-full bg-gray-800 select-none">
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
