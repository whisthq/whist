import React, { useState } from "react"
import classNames from "classnames"

import { WhistButton, WhistButtonState } from "@app/components/button"

const SelectBrowser = (props: {
  browsers: string[]
  onSelect: (browser: string) => void
}) => {
  return (
    <div className="w-full">
      <div className="font-semibold text-2xl text-gray-300">
        Import bookmarks and settings
      </div>
      <div className="m-auto mt-4 mb-4 text-gray-400 max-w-sm">
        Import your passwords, extensions, and bookmarks into Whist. You will
        need to submit your computer password twice to permit us to do this.
      </div>
      <div className="mt-8">
        <div className="bg-gray-800 px-4 py-2 rounded w-96 m-auto">
          <select
            className="bg-gray-800 font-semibold text-gray-300 outline-none mt-1 block w-full py-2 text-base border-gray-300 sm:text-sm"
            defaultValue={props.browsers[0]}
            onChange={(evt: any) => {
              props.onSelect(evt.target.value)
            }}
          >
            <option key={0} value="None">
              None, start from a clean slate
            </option>
            {props.browsers.map((browser: string, index: number) => (
              <option key={index + 1} value={browser}>
                {browser}
              </option>
            ))}
          </select>
        </div>
      </div>
    </div>
  )
}

const DownloadIcon = () => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    className="h-6 w-6 text-mint animate-pulse"
    viewBox="0 0 20 20"
    fill="currentColor"
  >
    <path
      fillRule="evenodd"
      d="M10.293 3.293a1 1 0 011.414 0l6 6a1 1 0 010 1.414l-6 6a1 1 0 01-1.414-1.414L14.586 11H3a1 1 0 110-2h11.586l-4.293-4.293a1 1 0 010-1.414z"
      clipRule="evenodd"
    />
  </svg>
)

const Importer = (props: {
  browsers: string[]
  onSubmit: (browser: string | undefined) => void
}) => {
  const [browser, setBrowser] = useState<string>(props.browsers?.[0])
  const [processing, setProcessing] = useState(false)

  const onSubmit = (browser: string) => {
    setProcessing(true)
    props.onSubmit(browser === "None" ? undefined : browser)
  }

  const onSelect = (browser: string) => {
    setBrowser(browser)
  }

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-gray-900",
        "justify-center font-body text-center px-12"
      )}
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <DownloadIcon />
      <div className="mt-8">
        <SelectBrowser
          browsers={props.browsers}
          onSelect={(browser: string) => onSelect(browser)}
        />
      </div>
      <div>
        <WhistButton
          contents="Continue"
          className="mt-4 px-12 w-96 mx-auto py-2 text-gray-300 text-gray-900 bg-mint"
          state={
            processing ? WhistButtonState.PROCESSING : WhistButtonState.DEFAULT
          }
          onClick={() => onSubmit(browser)}
        />
      </div>
    </div>
  )
}

export default Importer
