import React, { useEffect } from "react"
import { IntercomProvider, useIntercom } from "react-use-intercom"

const Support = (props: { userEmail: string | undefined }) => {
  const { show, update } = useIntercom()

  useEffect(() => {
    show()
  }, [])

  useEffect(() => {
    if (props.userEmail !== undefined) update({ email: props.userEmail })
  }, [props.userEmail])

  return (
    <div className="h-screen w-screen bg-white" id="hubspot">
      <div className="w-full h-8 draggable"></div>
    </div>
  )
}

const WithInterCom = (props: {
  onClose: () => void
  userEmail: string | undefined
}) => (
  <IntercomProvider
    appId={"v62favsy"}
    autoBoot={true}
    onHide={props.onClose}
    autoBootProps={{ hideDefaultLauncher: true, backgroundColor: "#4f35de" }}
  >
    <Support userEmail={props.userEmail} />
  </IntercomProvider>
)

export default WithInterCom
