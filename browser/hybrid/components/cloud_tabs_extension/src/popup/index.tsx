import * as React from "react"
import * as ReactDOM from "react-dom"
import { useState, useEffect } from "react"
import classNames from "classnames"

import Login from "./components/login"
import Waitlist from "./components/waitlist"

import Offline from "./containers/offline"
import Main from "./containers/main"
import Loading from "./containers/loading"

import { sendMessageToBackground } from "./utils/messaging"

import { PopupMessageType } from "@app/@types/messaging"

const App = () => {
  const [dark, setDark] = useState(true)
  const [isLoggedIn, setIsLoggedIn] = useState(true)
  const [host, setHost] = useState<string | undefined>(undefined)
  const [isSaved, setIsSaved] = useState(false)
  const [numberCloudTabs, setNumberCloudTabs] = useState(0)
  const [error, setError] = useState<string | undefined>(undefined)
  const [isCloudTab, setIsCloudTab] = useState<boolean | undefined>(undefined)
  const [isWaiting, setIsWaiting] = useState(false)
  const [networkError, setNetworkError] = useState(false)
  const [waitlisted, setWaitlisted] = useState(true)
  const [inviteError, setInviteError] = useState(false)
  const [loaded, setLoaded] = useState(false)
  const [networkInfo, setNetworkInfo] = useState(undefined)

  const parseHost = (host: string | undefined) => {
    if (host === undefined) return undefined
    const isLong = host.length > 15
    return `${host?.substring(0, 15)}${isLong ? "..." : ""}`
  }

  const fetchPopupData = async () => {
    const sleep = async (ms: number) =>
      await new Promise((resolve) => setTimeout(resolve, ms))
    let attempts = 0
    let _loaded = false
    while (!_loaded && attempts < 20) {
      attempts += 1
      sendMessageToBackground(
        PopupMessageType.FETCH_POPUP_DATA,
        undefined,
        ({
          error,
          isCloudTab,
          numberCloudTabs,
          isSaved,
          host,
          isWaiting,
          isLoggedIn,
          networkError,
          waitlisted,
          networkInfo,
        }) => {
          _loaded = true
          setLoaded(true)
          setError(error)
          setIsCloudTab(isCloudTab)
          setNumberCloudTabs(numberCloudTabs)
          setIsSaved(isSaved)
          setHost(parseHost(host))
          setIsWaiting(isWaiting)
          setIsLoggedIn(isLoggedIn)
          setNetworkError(networkError)
          setWaitlisted(waitlisted)
          setNetworkInfo(networkInfo)
        }
      )
      await sleep(250)
    }
  }

  useEffect(() => {
    void fetchPopupData()
    ;(chrome as any).braveTheme.getBraveThemeType((themeType: string) => {
      setDark(themeType === "Dark")
    })
  }, [])

  useEffect(() => {
    chrome.runtime.onMessage.addListener((message) => {
      if (message.type !== PopupMessageType.SEND_POPUP_DATA) return

      if (message.value.isLoggedIn !== undefined)
        setIsLoggedIn(message.value.isLoggedIn)

      if (message.value.isWaiting !== undefined)
        setIsWaiting(message.value.isWaiting)
    })
  }, [])

  const onToggled = (enabled: boolean) => {
    sendMessageToBackground(PopupMessageType.STREAM_TAB, { enabled }, () => {
      void fetchPopupData()
    })
  }

  const onSaved = (saved: boolean) => {
    sendMessageToBackground(PopupMessageType.SAVE_URL, { saved }, () => {
      void fetchPopupData()
    })
  }

  const onInviteCode = (code: string) => {
    setInviteError(false)
    sendMessageToBackground(
      PopupMessageType.INVITE_CODE,
      { code },
      ({ success }: { success: boolean }) => {
        success ? setWaitlisted(false) : setInviteError(true)
      }
    )
  }

  if (!loaded)
    return (
      <div className={dark ? "dark" : ""}>
        <Loading />
      </div>
    )

  if (networkError)
    return (
      <div className={dark ? "dark" : ""}>
        <Offline />
      </div>
    )

  if (!isLoggedIn)
    return (
      <div className={dark ? "dark" : ""}>
        <div className={classNames("w-screen bg-gray-100 dark:bg-gray-900")}>
          <Login
            onClick={() => {
              sendMessageToBackground(PopupMessageType.OPEN_LOGIN_PAGE)
            }}
          />
        </div>
      </div>
    )

  if (waitlisted)
    return (
      <div className={dark ? "dark" : ""}>
        <div className="w-screen bg-gray-100 dark:bg-gray-900">
          <Waitlist onClick={onInviteCode} error={inviteError} />
        </div>
      </div>
    )

  return (
    <div className={dark ? "dark" : ""}>
      <Main
        host={host}
        isCloudTab={isCloudTab}
        isWaiting={isWaiting}
        isSaved={isSaved}
        error={error}
        onToggled={onToggled}
        onSaved={onSaved}
        numberCloudTabs={numberCloudTabs}
        networkInfo={networkInfo}
      />
    </div>
  )
}

ReactDOM.render(<App />, document.getElementById("root"))
