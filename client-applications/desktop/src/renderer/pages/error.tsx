import React, { useState } from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"
import classNames from "classnames"
import { useMainState } from "@app/utils/ipc"
import TRIGGER from "@app/utils/triggers"

const Error = (props: {
  title: string
  text: string
  primaryButtonText: string
  secondaryButtonText: string
}) => {
  /*
        Description:
            Error pop-up

        Arguments:
            title (string): Title of error window
            text (string): Body text of error window
            primaryButtonText (string): Text to display on the big button
            secondaryButtonText (string): Text to display on the small button
    */

  const [processing, setProcessing] = useState(false)

  const [, setMainState] = useMainState()

  const relaunch = () =>
    setMainState({
      trigger: { name: TRIGGER.relaunchAction, payload: Date.now() },
    })

  const showPaymentWindow = () =>
    setMainState({
      trigger: { name: TRIGGER.showPaymentWindow, payload: null },
    })

  const showSignoutWindow = () =>
    setMainState({
      trigger: { name: TRIGGER.showSignoutWindow, payload: null },
    })

  const chooseEffect = (name: string) => {
    if (name === "Try Again") {
      return relaunch
    } else if (name === "Update Payment") {
      return showPaymentWindow
    } else if (name === "Sign Out") {
      return showSignoutWindow
    }
    return relaunch
  }
  const onPrimary = chooseEffect(props.primaryButtonText)
  const onSecondary = chooseEffect(props.secondaryButtonText)

  const onClick = () => {
    setProcessing(true)
    onPrimary()
  }

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center",
        "justify-center font-body text-center px-8"
      )}
    >
      <div className="mt-6 font-semibold text-2xl">{props.title}</div>
      <div className="my-2">{props.text}</div>
      <div className="mt-3 mb-1 w-full text-center">
        <FractalButton
          contents={props.primaryButtonText}
          className="px-12 mx-auto py-3"
          state={
            processing
              ? FractalButtonState.PROCESSING
              : FractalButtonState.DEFAULT
          }
          onClick={onClick}
        />
      </div>
      <button
        className="mx-auto mb-9 py-2 bg-none border-none text-gray outline-none"
        onClick={onSecondary}
        style={{ outline: "none" }}
      >
        {props.secondaryButtonText}
      </button>
    </div>
  )
}

export default Error
