import React from "react"
import { InlineWidget } from "react-calendly"

const Calendly = (props: { url: string }) => (
  <>
    <InlineWidget url={props.url} />
  </>
)

export default Calendly
