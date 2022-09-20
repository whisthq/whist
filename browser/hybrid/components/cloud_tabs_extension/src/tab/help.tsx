import React, { useEffect } from "react"
import { IntercomProvider, useIntercom } from "react-use-intercom"

const intercomId = "v62favsy"

const Support = (props: { userEmail: string | undefined }) => {
  const { show, update } = useIntercom()

  useEffect(() => {
    show()
  }, [])

  useEffect(() => {
    if (props.userEmail !== undefined) update({ email: props.userEmail })
  }, [props.userEmail])

  return (
    <div className="h-screen w-screen bg-gray-100 dark:bg-gray-900">
      <div className="w-full h-8 draggable"></div>
    </div>
  )
}

const WithInterCom = (props: {
  onClose: () => void
  userEmail: string | undefined
}) => (
  <IntercomProvider appId={intercomId} autoBoot={true} onHide={props.onClose}>
    <Support userEmail={props.userEmail} />
  </IntercomProvider>
)

export default WithInterCom
