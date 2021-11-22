import React from "react"

import { Progress } from "@app/components/html/progress"
import { FractalButton, FractalButtonState } from "@app/components/html/button"

const Network = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
  }
}) => {
  // Network test is in progress
  if (props.networkInfo.progress <= 100) {
    return (
      <div className="flex flex-col justify-center items-center bg-gray-800 h-screen text-center">
        <div className="w-full max-w-xs m-auto font-body">
          <div className="font-body text-xl font-semibold text-gray-300">
            Testing your Internet connection
          </div>
          <div className="mt-4">
            <Progress percent={props.networkInfo.progress ?? 10} color="blue" />
          </div>
        </div>
      </div>
    )
  }

  // Network test is finished and the network is good
  if (
    props.networkInfo.progress > 100 &&
    props.networkInfo.downloadMbps > 50 &&
    props.networkInfo.jitter < 20
  ) {
    return (
      <div className="flex flex-col justify-center items-center bg-gray-800 h-screen text-center">
        <div className="w-full max-w-xs m-auto font-body">
          <div className="font-body text-xl font-semibold text-gray-300">
            Your network connection is stable
          </div>
          <div className="mt-4">
            <Progress percent={100} color="blue" />
          </div>
        </div>
      </div>
    )
  }

  // Network may not be good
  return (
    <div className="flex flex-col justify-center items-center bg-gray-800 h-screen text-center">
      <div className="w-full max-w-xs m-auto font-body">
        <div className="font-body text-xl font-semibold text-gray-300">
          Your network connection may be unstable
        </div>
        <div className="mt-4">
          <FractalButton
            contents="Proceed Anyway"
            state={FractalButtonState.DEFAULT}
          />
        </div>
      </div>
    </div>
  )
}

export default Network
