import React, { useState } from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"
import classNames from "classnames"

const BaseError = (props: { title: string; text: string }) => {
  /*
        Description:
            Base error pop-up. Only contains the title and text.
        Arguments:
            title (string): Title of error window
            text (string): Body text of error window
    */
  return (
    <div>
      <div className="mt-6 font-semibold text-2xl text-gray-300">
        {props.title}
      </div>
      <div className="my-2 text-gray-500">{props.text}</div>
    </div>
  )
}

export const OneButtonError = (props: {
  title: string
  text: string
  primaryButtonText: string
  onPrimaryClick: () => void
}) => {
  /*
        Description:
            One button error pop-up. Includes the base error + a blue Fractal button
        Arguments:
            title (string): Title of error window
            text (string): Body text of error window
            primaryButtonText (string): Text to display on the big button
            onPrimary (string): Action to take on button press
    */
  const [processing, setProcessing] = useState(false)
  const onClick = () => {
    setProcessing(true)
    props.onPrimaryClick()
  }
  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-black bg-opacity-80",
        "justify-center font-body text-center px-8"
      )}
    >
      <BaseError title={props.title} text={props.text} />
      <div className="mt-4 mb-10 w-full text-center text-black">
        <FractalButton
          contents={props.primaryButtonText}
          className="px-12 mx-auto bg-mint py-3"
          state={
            processing
              ? FractalButtonState.PROCESSING
              : FractalButtonState.DEFAULT
          }
          onClick={onClick}
        />
      </div>
    </div>
  )
}

export const TwoButtonError = (props: {
  title: string
  text: string
  primaryButtonText: string
  secondaryButtonText: string
  onPrimaryClick: () => void
  onSecondaryClick: () => void
}) => {
  /*
        Description:
            Two button error pop-up. Includes the base error and two buttons.
        Arguments:
            title (string): Title of error window
            text (string): Body text of error window
            primaryButtonText (string): Text to display on the big button
            secondaryButtonText (string): Text to display on the secondary button
            onPrimary (string): Action to take on primary button press
            onSecondary (string): Action to take on secondary button press
    */
  const [processing, setProcessing] = useState(false)
  const onClick = () => {
    setProcessing(true)
    props.onPrimaryClick()
  }
  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-black bg-opacity-80",
        "justify-center font-body text-center px-8"
      )}
    >
      <BaseError title={props.title} text={props.text} />
      <div className="mt-3 mb-1 w-full text-center">
        <FractalButton
          contents={props.primaryButtonText}
          className="px-12 mx-auto py-3 bg-mint text-black"
          state={
            processing
              ? FractalButtonState.PROCESSING
              : FractalButtonState.DEFAULT
          }
          onClick={onClick}
        />
      </div>
      <button
        className="mx-auto mb-6 py-2 bg-none border-none text-gray-300 underline outline-none"
        onClick={props.onSecondaryClick}
        style={{ outline: "none" }}
      >
        {props.secondaryButtonText}
      </button>
    </div>
  )
}
