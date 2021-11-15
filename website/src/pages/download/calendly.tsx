import React from "react"
import { InlineWidget } from "react-calendly"

const CALENDLY_URL = "https://calendly.com/whist-ming/onboarding"

const Calendly = (props: { url: string }) => (
  <>
    <InlineWidget url={props.url} />
  </>
)

export default Calendly
