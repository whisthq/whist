import React from "react"
import { chain } from "lodash"
import ReactDOM from "react-dom"

import Update from "@app/renderer/pages/update"
import Error from "@app/renderer/pages/error"
import { WindowHashUpdate } from "@app/utils/constants"
import {
  NoAccessError,
  UnauthorizedError,
  ProtocolError,
  MandelboxError,
  AuthError,
  NavigationError,
} from "@app/utils/error"

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

  const errorContinue = () =>
    setMainState({ trigger: { name: "relaunch", payload: Date.now() } })

  if (show === WindowHashUpdate) return <Update />
  if (show === AuthError.hash) {
    return (
      <Error
        title={AuthError.title}
        text={AuthError.text}
        onClick={errorContinue}
      />
    )
  }
  if (show === NoAccessError.hash) {
    return (
      <Error
        title={NoAccessError.title}
        text={NoAccessError.text}
        onClick={errorContinue}
      />
    )
  }
  if (show === UnauthorizedError.hash) {
    return (
      <Error
        title={UnauthorizedError.title}
        text={UnauthorizedError.text}
        onClick={errorContinue}
      />
    )
  }
  if (show === MandelboxError.hash) {
    return (
      <Error
        title={MandelboxError.title}
        text={MandelboxError.text}
        onClick={errorContinue}
      />
    )
  }
  if (show === ProtocolError.hash) {
    return (
      <Error
        title={ProtocolError.title}
        text={ProtocolError.text}
        onClick={errorContinue}
      />
    )
  }
  return (
    <Error
      title={NavigationError.title}
      text={NavigationError.text}
      onClick={errorContinue}
    />
  )
}

// TODO: actually pass version number through IPC.
const WindowBackground = (props: any) => {
  const win = window as unknown as { VERSION: number }
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

ReactDOM.render(
  <WindowBackground>
    <RootComponent />
  </WindowBackground>,
  document.getElementById("root")
)
