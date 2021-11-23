import React, { memo, useState } from "react"
import { Widget } from "@typeform/embed-react"

const Typeform = (props: {
  id: string
  email: string
  onSubmit: () => void
}) => {
  const [submitted, setSubmitted] = useState(false)

  const onSubmit = () => {
    setSubmitted(true)
    props.onSubmit()
  }

  if ((props.email ?? "") !== "" && !submitted) {
    return (
      <Widget
        id={props.id}
        className="w-screen h-screen bg-gray-800"
        onSubmit={onSubmit}
        hidden={{ email: props.email }}
      />
    )
  } else {
    return <div className="relative w-screen h-screen bg-gray-800"></div>
  }
}

// We memoize this component because without it, the embeded typeform URL will reload on every
// component refresh higher up in the tree
export default memo(Typeform)
