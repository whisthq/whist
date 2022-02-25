import React, { useState } from "react"
import classNames from "classnames"

import Dropdown from "@app/components/dropdown"
import { WhistButton, WhistButtonState } from "@app/components/button"

const Importer = (props: {
  browsers: string[]
  onSubmit: (browser: string | undefined) => void
}) => {
  const [browser, setBrowser] = useState("")
  const [processing, setProcessing] = useState(false)

  const onSelect = (b: string) => {
    setBrowser(b)
  }

  const onSubmit = (b: string) => {
    setProcessing(true)
    props.onSubmit(b)
  }

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
          Please select which browser you would like to import your cookies,
          bookmarks, passwords, and extensions from so you can pick up right
          where you left off.
        </div>
        <div className="mt-8 max-w-sm m-auto">
          <Dropdown options={props.browsers} onSelect={onSelect} />
        </div>
        <div>
          <WhistButton
            contents="Continue"
            className="mt-4 px-12 w-96 mx-auto py-2 text-gray-300 text-gray-900 bg-blue-light"
            state={
              processing
                ? WhistButtonState.PROCESSING
                : WhistButtonState.DEFAULT
            }
            onClick={() => onSubmit(browser)}
          />
        </div>
        <button
          className="relative top-12 text-blue-light font-bold outline-none bg-none"
          onClick={() => onSubmit("")}
        >
          Skip for now
        </button>
      </div>
    </div>
  )
}

export default Importer
