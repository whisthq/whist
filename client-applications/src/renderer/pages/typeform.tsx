import React from "react"
import { Widget } from "@typeform/embed-react"

const Typeform = (props: {
  id: string
  email: string
  onSubmit: () => void
}) => {
  if ((props.email ?? "") !== "") {
    return (
      <Widget
        id={props.id}
        className="w-screen h-screen"
        onSubmit={props.onSubmit}
        hidden={{ email: props.email }}
      />
    )
  } else {
    return <div className="relative w-screen h-screen bg-opacity-0"></div>
  }
}

export default Typeform
