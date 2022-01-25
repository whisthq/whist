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
  const onKeyDown = (evt: any) => {
    if (evt.key === "Enter" && props?.onSubmit !== undefined) props.onSubmit()
  }

  return (
    <div
      onKeyDown={onKeyDown}
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none bg-gray-900"
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="mt-24">
        <NetworkComponent
          networkInfo={props.networkInfo}
          onSubmit={props.onSubmit}
          withText={true}
          withButton={true}
        />
      </div>
    </div>
  )
}

export default Network
