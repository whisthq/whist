import React, { useState, useEffect } from "react"
import sample from "lodash.sample"

import Logo from "@app/components/icons/logo.svg"
import { Progress } from "@app/components/progress"
import Spinner from "@app/components/spinner"
import Check from "@app/components/icons/check"
import Warning from "@app/components/icons/warning"

import {
  MAX_ACCEPTABLE_JITTER_MS,
  MAX_ACCEPTABLE_PING_MS,
  MIN_ACCEPTABLE_DOWNLOAD_MBPS,
} from "@app/constants/network"

const loadingMessage = sample([
  "to view settings when launched",
  "for live chat support when launched",
  "to import tabs from another browser",
])

const InternetNotification = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
    ping: number
  }
}) => {
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    setTimeout(() => setLoading(false), 5000)
  })

  if (
    props.networkInfo?.downloadMbps < MIN_ACCEPTABLE_DOWNLOAD_MBPS &&
    !loading
  ) {
    return (
      <div className="w-full flex space-x-3 justify-center">
        <div className="w-3 h-3 relative text-red-400">
          <Warning />
        </div>
        <div>
          Your bandwidth ({props.networkInfo.downloadMbps} Mbps) is low. You may
          experience latency and stuttering.
        </div>
      </div>
    )
  }

  if (props.networkInfo?.ping > MAX_ACCEPTABLE_PING_MS && !loading) {
    return (
      <div className="w-full flex space-x-3 justify-center">
        <div className="w-3 h-3 relative text-red-400">
          <Warning />
        </div>
        <div>
          Your ping ({props.networkInfo.ping} ms) is high. You may experience
          latency and stuttering.
        </div>
      </div>
    )
  }

  if (props.networkInfo?.jitter > MAX_ACCEPTABLE_JITTER_MS && !loading) {
    return (
      <div className="w-full flex space-x-3 justify-center">
        <div className="w-3 h-3 relative text-red-400">
          <Warning />
        </div>
        <div>
          Your network jitter ({props.networkInfo.jitter} ms) is high. You may
          experience latency and stuttering.
        </div>
      </div>
    )
  }

  if (loading) {
    return (
      <div className="w-full flex space-x-3 justify-center">
        <div className="w-3 h-3 relative top-0.5">
          <Spinner />
        </div>
        <div>Testing your Internet connection</div>
      </div>
    )
  }

  return (
    <div className="w-full flex space-x-3 justify-center">
      <div className="relative bottom-0.5 text-blue-light">
        <Check />
      </div>
      <div className="text-gray-300">Your Internet is ready for Whist</div>
    </div>
  )
}

const Loading = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
    ping: number
  }
}) => {
  return (
    <div className="flex flex-col h-screen bg-gray-900 w-full font-body">
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="w-full text-center pt-16">
        <img src={Logo} className="w-20 h-20 mx-auto" />
        <div className="mt-12 text-xl text-gray-300 font-bold">
          Whist is launching
        </div>
      </div>
      <div className="mt-4 w-full max-w-xs mx-auto">
        <Progress
          percent={0}
          className="h-2"
          mockProgress={true}
          mockSpeed={2.5}
        />
      </div>
      <div className="w-full text-center">
        <div className="flex justify-center pointer-events-none space-x-1 mt-2">
          <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
            ⌘
          </kbd>
          <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
            J
          </kbd>
        </div>
        <div className="text-gray-500 text-sm mt-2">{loadingMessage}</div>
      </div>
      <div className="absolute bottom-4 left-0 right-0 w-full bg-gray-800 bg-opacity-50 px-8 py-4 text-center text-gray-400 text-sm max-w-md rounded m-auto">
        <InternetNotification networkInfo={props.networkInfo} />
      </div>
    </div>
  )
}

export default Loading
