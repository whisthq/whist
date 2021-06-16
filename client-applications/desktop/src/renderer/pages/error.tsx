import React, { useState } from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"
import classNames from "classnames"

const Error = (props: {
  title: string
  text: string
  buttonText: string
  onContinue: () => void
  onSignout: () => void
}) => {
  /*
        Description:
            Error pop-up

        Arguments:
            title (string): Title of error window
            text (string): Body text of error window
            buttonText (string): Text to display on the button
            onContinue (() => void): Function to execute when window button is pressed
            onSignout (() => void): Function to execute when signout link is pressed
    */

  const [processing, setProcessing] = useState(false)

  const onClick = () => {
    setProcessing(true)
    props.onContinue()
  }

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center",
        "justify-center font-body text-center px-8"
      )}
    >
      <div className="font-semibold text-2xl">{props.title}</div>
      <div className="mt-2 mb-4">{props.text}</div>
      <div className="w-full text-center">
        <FractalButton
          contents={props.buttonText}
          className="mt-4 px-12 mx-auto py-3"
          state={
            processing
              ? FractalButtonState.PROCESSING
              : FractalButtonState.DEFAULT
          }
          onClick={onClick}
        />
      </div>
      <button
        className="mx-auto py-3 bg-none border-none text-gray outline-none"
        onClick={props.onSignout}
        style={{ outline: "none" }}
      >
        Sign Out
      </button>
    </div>
  )
}

export default Error
