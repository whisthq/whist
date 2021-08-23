import React from "react"

import { FaWifi } from "react-icons/fa"

const Warning = (props: { text: string }) => (
  <div className="bg-gray-900 font-semibold tracking-wider font-body text-gray-100 text-sm w-screen h-screen pt-2 flex justify-center content-center rounded">
    <div className="relative top-1">
      <FaWifi className="text-green-600" />
    </div>
    <div className="ml-3 mt-1">{props.text}</div>
  </div>
)

export default Warning
