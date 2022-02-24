import React, { useState } from "react"

import { WhistButton, WhistButtonState } from "@app/components/button"
import Right from "@app/components/icons/right"
import Left from "@app/components/icons/left"
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
      <div className="mt-6 font-semibold text-2xl text-gray-300 max-w-sm m-auto">
        {props.title}
      </div>
      <div className="my-2 text-gray-500 max-w-md mb-4">{props.text}</div>
    </div>
  )
}

export const OneButtonError = (props: {
  title: string
  text: string
  primaryButtonText: string
  onPrimaryClick: () => void
  onRequestHelp?: () => void
}) => {
  /*
        Description:
            One button error pop-up. Includes the base error + a blue Whist button
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
        "flex flex-col h-screen items-center bg-gray-900",
        "justify-center font-body text-center px-8"
      )}
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <BaseError title={props.title} text={props.text} />
      <div className="mt-4 mb-10 w-full text-center text-gray-900 flex justify-center space-x-4">
        <WhistButton
          contents={
            <div className="flex space-x-1">
              <Left />
              <div>{props.primaryButtonText}</div>
            </div>
          }
          className="px-12 bg-blue-light py-3"
          state={
            processing ? WhistButtonState.PROCESSING : WhistButtonState.DEFAULT
          }
          onClick={onClick}
        />
        {props.onRequestHelp !== undefined && (
          <WhistButton
            contents={
              <div className="flex space-x-1">
                <Right />
                <div>Report A Bug</div>
              </div>
            }
            className="px-10 bg-blue-light py-3 bg-opacity-0 border border-gray-700 text-gray-300 hover:bg-opacity-0"
            state={WhistButtonState.DEFAULT}
            onClick={props.onRequestHelp}
          />
        )}
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
  onRequestHelp: () => void
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
        "flex flex-col h-screen items-center bg-gray-900",
        "justify-center font-body text-center px-8"
      )}
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <BaseError title={props.title} text={props.text} />
      <div className="mt-4 mb-4 w-full text-center text-gray-900 flex justify-center space-x-4">
        <WhistButton
          contents={
            <div className="flex space-x-1">
              <Left />
              <div>{props.primaryButtonText}</div>
            </div>
          }
          className="px-12 bg-blue-light py-3"
          state={
            processing ? WhistButtonState.PROCESSING : WhistButtonState.DEFAULT
          }
          onClick={onClick}
        />
        <WhistButton
          contents={
            <div className="flex space-x-1">
              <Right />
              <div>Report A Bug</div>
            </div>
          }
          className="px-10 bg-blue-light py-3 bg-opacity-0 border border-gray-700 text-gray-300 hover:bg-opacity-0"
          state={WhistButtonState.DEFAULT}
          onClick={props.onRequestHelp}
        />
      </div>
      <button
        className="mx-auto mb-6 py-2 bg-none border-none text-gray-500 underline outline-none"
        onClick={props.onSecondaryClick}
        style={{ outline: "none" }}
      >
        {props.secondaryButtonText}
      </button>
    </div>
  )
}
