import React, { useState, useEffect } from "react"
import classNames from "classnames"

import Dropdown from "@app/components/dropdown"
import { WhistButton, WhistButtonState } from "@app/components/button"

const Importer = (props: {
  browsers: string[] | undefined
  onSubmit: (browser: string | undefined) => void
  allowSkip: boolean
}) => {
  console.log("browsers are", props.browsers)

  const [browser, setBrowser] = useState("")
  const [processing, setProcessing] = useState(false)

  const onSelect = (b: string) => {
    setBrowser(b)
  }

  const onSubmit = (shouldImport: boolean) => {
    setProcessing(true)
    setTimeout(() => {
      props.onSubmit(shouldImport ? browser : "")
    }, 2000)
  }

  useEffect(() => {
    if (props.browsers !== undefined && props.browsers.length > 0) {
      setBrowser(props.browsers[0])
    }
  }, [props.browsers])

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
          {props.browsers === undefined || props.browsers.length === 0 ? (
            <>
              <Dropdown
                options={["No supported browsers found :("]}
                onSelect={() => {}}
              />
            </>
          ) : (
            <>
              <Dropdown options={props.browsers ?? []} onSelect={onSelect} />
            </>
          )}
        </div>
        <div>
          <WhistButton
            contents="Continue"
            className="mt-4 px-12 w-96 mx-auto py-2 text-gray-300 text-gray-900 bg-blue-light"
            state={
              props.browsers === undefined || props.browsers.length === 0
                ? WhistButtonState.DISABLED
                : processing
                ? WhistButtonState.PROCESSING
                : WhistButtonState.DEFAULT
            }
            onClick={() => onSubmit(true)}
          />
        </div>
        {props.allowSkip && (
          <div className="relative top-12">
            <button
              className="text-blue-light font-bold outline-none bg-none"
              onClick={() => onSubmit(false)}
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
