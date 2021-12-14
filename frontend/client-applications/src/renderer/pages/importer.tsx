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
        Import your cookies, bookmarks, extensions and passwords
      </div>
      <div className="m-auto mt-2 mb-4 text-gray-400 max-w-xl">
        You will be asked for your computer password twice when we copy your
        browser data.
      </div>
      <div className="mt-8">
        <div className="bg-gray-900 px-4 py-2 rounded w-96 m-auto">
          <select
            id="browser"
            name="browser"
            className="bg-gray-900 font-semibold text-gray-300 outline-none mt-1 block w-full py-2 text-base border-gray-300 sm:text-sm"
            defaultValue={props.browsers?.[0]}
            onChange={(evt: any) => props.onSelect(evt.target.value)}
          >
            <option value={undefined}>None, start from a clean slate</option>
            {props.browsers.map((browser: string, index: number) => (
              <option key={index} value={browser}>
                {browser}
              </option>
            ))}
          </select>
        </div>
      </div>
    </div>
  )
}

const Importer = (props: {
  browsers: string[]
  onSubmit: (browser: string | undefined) => void
}) => {
  const [browser, setBrowser] = useState<string | undefined>(
    props.browsers?.[0]
  )
  const [processing, setProcessing] = useState(false)

  const onSubmit = (browser: string | undefined) => {
    setProcessing(true)
    props.onSubmit(browser)
  }

  const onSelect = (browser: string | undefined) => {
    setBrowser(browser)
  }

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-gray-800",
        "justify-center font-body text-center px-12"
      )}
    >
      <SelectBrowser
        browsers={props.browsers}
        onSelect={(browser: string) => onSelect(browser)}
      />
      <div>
        <WhistButton
          contents="Continue"
          className="mt-4 px-12 w-96 mx-auto py-2 text-gray-300 hover:text-black"
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
