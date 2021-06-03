import React from "react"
import { chain, keys } from "lodash"
import ReactDOM from "react-dom"

import Update from "@app/renderer/pages/update"
import Error from "@app/renderer/pages/error"
import Signout from "@app/renderer/pages/signout"

import { WindowHashUpdate, WindowHashSignout } from "@app/utils/constants"
import { fractalError } from "@app/utils/error"
import { useMainState } from "@app/utils/ipc"
import TRIGGER from "@app/utils/triggers"

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
    setMainState({
      trigger: { name: TRIGGER.relaunchAction, payload: Date.now() },
    })

  const clearCache = () =>
    setMainState({ trigger: { name: TRIGGER.clearCacheAction, payload: null } })

  const showSignoutWindow = () =>
    setMainState({
      trigger: { name: TRIGGER.showSignoutWindow, payload: null },
    })

  if (show === WindowHashUpdate) return <Update />
  if (show === WindowHashSignout) return <Signout onClick={clearCache} />
  if (keys(fractalError).includes(show))
    return (
      <Error
        title={fractalError[show].title}
        text={fractalError[show].text}
        onContinue={errorContinue}
        onSignout={showSignoutWindow}
      />
    )
  return (
    <Error
      title={fractalError.NAVIGATION_ERROR.title}
      text={fractalError.NAVIGATION_ERROR.text}
      onContinue={errorContinue}
      onSignout={clearCache}
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
