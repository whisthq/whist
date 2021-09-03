import React from "react"
import { Widget } from "@typeform/embed-react"

const Typeform = (props: {
  id: string
  email: string
  onSubmit: () => void
}) => {
  return (
    <Widget
      id={props.id}
      className="w-screen h-screen"
      onSubmit={props.onSubmit}
      hidden={{ email: props.email }}
    />
  )
}

export default Typeform
