import React, { useState } from "react"
import classNames from "classnames"

import Dropdown from "@app/components/dropdown"
import Landing from "@app/components/templates/landing"
import Payment from "@app/components/icons/payment"

const Importer = (props: {
  browsers: string[]
  onSubmit: (browser: string | undefined) => void
  allowSkip: boolean
  mode: string
}) => {
  const [showWelcome, setShowWelcome] = useState(props.mode === "onboarding")
  const [processing, setProcessing] = useState(false)

  const onSubmit = (browser: string) => {
    setProcessing(true)
    props.onSubmit(browser)
  }

  if (showWelcome)
    return (
      <Landing
        logo={
          <div className="w-24 h-24 m-auto animate-fade-in-up opacity-0">
            <Payment />
          </div>
        }
        title="Payment Successful"
        subtitle="Now let's finish setting up Whist"
        onSubmit={() => {
          setShowWelcome(false)
        }}
      />
    )

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-gray-900",
        "justify-center font-body text-center px-12"
      )}
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="max-w-md m-auto">
        <div className="text-2xl text-gray-300 font-bold">
          Do you want to import your bookmarks and settings?
        </div>
        <div className="text-gray-500 mt-4">
          Pick up right where you left off by importing your bookmarks,
          extensions, and passwords from another browser. We currently support
          Chrome, Brave, and Opera.
        </div>
        <div className="mt-8 max-w-sm m-auto">
          <Dropdown
            options={
              props.browsers.length === 0
                ? ["No supported browsers found :("]
                : props.browsers
            }
            onSubmit={
              props.browsers.length === 0
                ? () => onSubmit("")
                : (value) => onSubmit(value)
            }
            submitButton={
              <input
                type="submit"
                value="Continue"
                className="mt-4 px-12 w-96 mx-auto py-2 text-gray-300 text-gray-900 bg-blue-light rounded cursor-pointer py-4 font-bold"
              />
            }
          />
        </div>
        {props.allowSkip && (
          <div className="relative top-12">
            <button
              className="text-blue-light font-bold outline-none bg-none"
              onClick={() => onSubmit("")}
              disabled={processing}
            >
              Skip for now
            </button>
            <div className="mt-2 text-xs text-gray-500">
              You can import later from{" "}
              <kbd className="bg-gray-700 rounded p-1 mx-1">Cmd</kbd>
              <kbd className="bg-gray-700 rounded p-1">J</kbd>
            </div>
          </div>
        )}
      </div>
    </div>
  )
}

export default Importer
