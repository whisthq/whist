import React from "react"

import { FaExclamationTriangle } from "react-icons/fa"

const Warning = (props: { text: string }) => (
  <div className="bg-black bg-opacity-80 font-semibold tracking-wider font-body text-gray-100 text-sm w-screen h-screen pt-2 flex justify-center content-center">
    <div className="relative top-1">
      <FaExclamationTriangle className="text-red-400" />
    </div>
    <div className="ml-3 mt-1">{props.text}</div>
  </div>
)

export default Warning
