import * as React from "react"
import * as ReactDOM from "react-dom"
import { useState, useEffect } from "react"

import WithInterCom from "./help"

const App = () => {
  const [dark, setDark] = useState(true)

  useEffect(() => {
    ;(chrome as any).braveTheme.getBraveThemeType((themeType: string) => {
      setDark(themeType === "Dark")
    })
  }, [])

  return (
    <div className={dark ? "dark" : ""}>
      <WithInterCom onClose={() => {}} userEmail={undefined} />
    </div>
  )
}

ReactDOM.render(<App />, document.getElementById("root"))
