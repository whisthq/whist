import React, { useState, useEffect } from "react"
import { IntercomProvider, useIntercom } from "react-use-intercom"

const Support = (props: { userEmail: string | undefined }) => {
  const { show, update } = useIntercom()
  const [dark, setDark] = useState(true)

  useEffect(() => {
    ;(chrome as any).braveTheme.getBraveThemeType((theme: string) =>
      setDark(theme === "Dark")
    )
    ;(chrome as any).braveTheme.onBraveThemeTypeChanged.addListener(
      (theme: string) => setDark(theme === "Dark")
    )
  }, [])

  useEffect(() => {
    show()
  }, [])

  useEffect(() => {
    if (props.userEmail !== undefined) update({ email: props.userEmail })
  }, [props.userEmail])

  return (
    <div
      className="h-screen w-screen"
      style={{
        background: dark ? "#303443" : "#f3f3f3",
      }}
    >
      <div className="w-full h-8 draggable"></div>
    </div>
  )
}

const WithInterCom = (props: {
  onClose: () => void
  userEmail: string | undefined
}) => (
  <IntercomProvider appId="v62favsy" autoBoot={true} onHide={props.onClose}>
    <Support userEmail={props.userEmail} />
  </IntercomProvider>
)

export default WithInterCom
