import React from "react"

import { Network as NetworkComponent } from "@app/components/network"

const Network = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
  }
  onSubmit?: () => void
}) => {
  return (
    <div className="flex flex-col justify-center items-center bg-gray-800 h-screen text-center draggable">
      <NetworkComponent
        networkInfo={props.networkInfo}
        onSubmit={props.onSubmit}
        withText={true}
      />
    </div>
  )
}

export default Network
