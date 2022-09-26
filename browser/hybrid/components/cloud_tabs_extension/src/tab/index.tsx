import * as React from "react"
import * as ReactDOM from "react-dom"
import { useState, useEffect } from "react"

import WithInterCom from "./help"
import { HelpScreenMessageType } from "@app/@types/messaging"

const App = () => {
  const [dark, setDark] = useState(true)
  const [userEmail, setUserEmail] = useState(undefined)

  useEffect(() => {
    ;(chrome as any).braveTheme.getBraveThemeType((themeType: string) => {
      setDark(themeType === "Dark")
    })
  }, [])

  void chrome.runtime.sendMessage({
    type: HelpScreenMessageType.HELP_SCREEN_OPENED,
  })
  useEffect(() => {
    chrome.runtime.onMessage.addListener((message) => {
      if (message.type !== HelpScreenMessageType.HELP_SCREEN_USER_EMAIL) return
      setUserEmail(message.value)
    })
  }, [])

  return (
    <div className={dark ? "dark" : ""}>
      <WithInterCom onClose={() => {}} userEmail={userEmail} />
    </div>
  )
}

ReactDOM.render(<App />, document.getElementById("root"))
