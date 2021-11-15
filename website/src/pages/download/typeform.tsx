import React, { useState } from "react"
import { Widget } from "@typeform/embed-react"

const Typeform = (props: { id: string; onSubmit: () => void }) => {
  const [submitted, setSubmitted] = useState(false)

  const onSubmit = () => {
    setSubmitted(true)
    props.onSubmit()
  }

  if (!submitted) {
    return (
      <Widget id={props.id} className="w-full h-full" onSubmit={onSubmit} />
    )
  } else {
    return (
      <div className="relative bg-gray-900">
        <div className="flex justify-center items-center m-auto pt-72">
          <div className="mt-12 animate-spin rounded-full h-12 w-12 border-t-2 border-b-2 border-blue"></div>
        </div>
      </div>
    )
  }
}

export default Typeform
