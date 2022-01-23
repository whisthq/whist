import React from "react"

import { Network as NetworkComponent } from "@app/components/network"

const Loading = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
  }
}) => {
  return (
    <div className="flex flex-col h-screen bg-gray-900 w-full font-body">
      <div className="absolute top-0 left-0 w-full h-6 draggable"></div>
      <div className="w-full text-center pt-10">
        <div className="mt-4 font-semibold text-xl text-gray-300">
          Whist is connecting to servers
        </div>
        <div className="mt-6 w-full">
          <NetworkComponent networkInfo={props.networkInfo} withText={false} />
        </div>
      </div>
      <div className="w-full text-center">
        <div className="flex justify-center pointer-events-none space-x-1 mt-4">
          <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
            ⌘
          </kbd>
          <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
            J
          </kbd>
        </div>
        <div className="text-gray-500 text-sm mt-2">
          to view settings when connected
        </div>
      </div>
    </div>
  )
}

export default Loading
