import React from "react"
import { Switch, Route } from "react-router-dom"
import { Router } from "react-router"
import { chain } from "lodash"
import ReactDOM from "react-dom"
import { loadStripe } from "@stripe/stripe-js"
import { Elements } from "@stripe/react-stripe-js"

import config from "@app/config/environment"
import Auth from "@app/renderer/pages/auth"
import Update from "@app/renderer/pages/update"
import Error from "@app/renderer/pages/error"
import {
  AuthErrorTitle,
  AuthErrorText,
  ContainerCreateErrorTitleInternal,
  ContainerCreateErrorTextInternal,
  ContainerCreateErrorTitleNoAccess,
  ContainerCreateErrorTextNoAccess,
  ContainerCreateErrorTitleUnauthorized,
  ContainerCreateErrorTextUnauthorized,
  containerPollingErrorTitle,
  containerPollingErrorText,
  ProtocolErrorTitle,
  ProtocolErrorText,
  WindowHashAuth,
  WindowHashUpdate,
  WindowHashAuthError,
  WindowHashCreateContainerErrorNoAccess,
  WindowHashCreateContainerErrorUnauthorized,
  WindowHashCreateContainerErrorInternal,
  WindowHashAssignContainerError,
  WindowHashProtocolError,
  NavigationErrorTitle,
  NavigationErrorText,
} from "@app/utils/constants"

import { browserHistory } from "@app/utils/history"
import { useMainState } from "@app/utils/ipc"

// Electron has no way to pass data to a newly launched browser
// window. To avoid having to maintain multiple .html files for
// each kind of window, we pass a constant across to the renderer
// thread as a query parameter to determine which component
// should be rendered.

// If no query parameter match is found, we default to a
// generic navigation error window.
const show = chain(window.location.search.substring(1))
  .split("=")
  .chunk(2)
  .fromPairs()
  .get("show")
  .value()

const RootComponent = () => {
  const [, setMainState] = useMainState()

  const errorContinue = () => setMainState({ errorRelaunchRequest: Date.now() })

  if (show === WindowHashAuth) {
    return (
      <Router history={browserHistory}>
        <Switch>
          <Route path="/" component={Auth} />
        </Switch>
      </Router>
    )
  }
  if (show === WindowHashUpdate) return <Update />
  if (show === WindowHashAuthError) {
    return (
      <Error
        title={AuthErrorTitle}
        text={AuthErrorText}
        onClick={errorContinue}
      />
    )
  }
  if (show === WindowHashCreateContainerErrorNoAccess) {
    return (
      <Error
        title={ContainerCreateErrorTitleNoAccess}
        text={ContainerCreateErrorTextNoAccess}
        onClick={errorContinue}
      />
    )
  }
  if (show === WindowHashCreateContainerErrorUnauthorized) {
    return (
      <Error
        title={ContainerCreateErrorTitleUnauthorized}
        text={ContainerCreateErrorTextUnauthorized}
        onClick={errorContinue}
      />
    )
  }
  if (show === WindowHashCreateContainerErrorInternal) {
    return (
      <Error
        title={ContainerCreateErrorTitleInternal}
        text={ContainerCreateErrorTextInternal}
        onClick={errorContinue}
      />
    )
  }
  if (show === WindowHashAssignContainerError) {
    return (
      <Error
        title={containerPollingErrorTitle}
        text={containerPollingErrorText}
        onClick={errorContinue}
      />
    )
  }
  if (show === WindowHashProtocolError) {
    return (
      <Error
        title={ProtocolErrorTitle}
        text={ProtocolErrorText}
        onClick={errorContinue}
      />
    )
  }
  return (
    <Error
      title={NavigationErrorTitle}
      text={NavigationErrorText}
      onClick={errorContinue}
    />
  )
}

// TODO: actually pass version number through IPC.
const WindowBackground = (props: any) => {
  const win = (window as unknown) as { VERSION: number }
  const version = win.VERSION
  return (
    <div className="relative w-full h-full">
      <div
        className="bg-white absolute flex flex-col-reverse items-center w-full h-full"
        style={{ zIndex: -10 }}
      >
        <p className="font-body font-light text-gray-200 py-4">{version}</p>
      </div>
      {props.children}
    </div>
  )
}
const stripePromise = loadStripe(config.stripe.STRIPE_PUBLISH_KEY)

ReactDOM.render(
  <Elements stripe={stripePromise}>
    <WindowBackground>
      <RootComponent />
    </WindowBackground>
  </Elements>,
  document.getElementById("root")
)
