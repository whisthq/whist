import React from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"
import classNames from "classnames"

const Error = (props: {
  title: string
  text: string
  onContinue: () => void
  onSignout: () => void
}) => {
  /*
        Description:
            Error pop-up

        Arguments:
            title (string): Title of error window
            text (string): Body text of error window
            onClick (() => void): Function to execute when window button is pressed
    */

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center",
        "justify-center font-body text-center"
      )}
    >
      <div className="font-semibold text-2xl">{props.title}</div>
      <div className="mt-2 mb-4">{props.text}</div>
      <div className="w-full text-center">
        <FractalButton
          contents="Try Again"
          className="mt-4 px-12 mx-auto py-3"
          state={FractalButtonState.DEFAULT}
          onClick={props.onContinue}
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
