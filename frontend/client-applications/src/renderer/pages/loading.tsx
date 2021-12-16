import React from "react"
import classNames from "classnames"

import { Network as NetworkComponent } from "@app/components/network"

const Loading = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
  }
}) => {
  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-gray-800 w-full",
        "justify-center font-body text-center"
      )}
    >
      <div className="mt-6 font-semibold text-2xl text-gray-300">
        Whist is loading
      </div>
      <div className="text-gray-500 mt-2">
        Please allow a few seconds while we connect to our servers
      </div>
      <div className="mt-6">
        <NetworkComponent networkInfo={props.networkInfo} withText={false} />
      </div>
    </div>
  )
}

export default Loading
