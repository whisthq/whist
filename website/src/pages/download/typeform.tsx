import React from "react"
import { Widget } from "@typeform/embed-react"

const Typeform = (props: { id: string }) => {
  return <Widget id={props.id} className="w-full h-full" />
}

export default Typeform
