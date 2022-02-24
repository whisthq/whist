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
      <Dropdown options={props.browsers} onSelect={onSelect} />
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
