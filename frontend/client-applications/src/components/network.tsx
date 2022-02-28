import React from "react"
import classNames from "classnames"

import { Progress } from "@app/components/progress"
import { WhistButton, WhistButtonState } from "@app/components/button"

const MAX_ACCEPTABLE_PING_MS = 50
const MAX_ACCEPTABLE_JITTER_MS = 15
const MIN_ACCEPTABLE_DOWNLOAD_MBPS = 30

enum NetworkTestState {
  IN_PROGRESS,
  NETWORK_GOOD,
  NETWORK_NOT_GOOD,
}

const getNetworkTestState = (
  networkInfo:
    | {
        jitter: number
        progress: number
        downloadMbps: number
        ping: number
      }
    | undefined
) => {
  // Network test is in progress
  if (networkInfo === undefined) return NetworkTestState.IN_PROGRESS

  // Network test is finished and the network is good
  if (
    networkInfo.downloadMbps >= MIN_ACCEPTABLE_DOWNLOAD_MBPS &&
    networkInfo.jitter < MAX_ACCEPTABLE_JITTER_MS &&
    networkInfo.ping < MAX_ACCEPTABLE_PING_MS
  )
    return NetworkTestState.NETWORK_GOOD

  // Network may not be good
  return NetworkTestState.NETWORK_NOT_GOOD
}

const Subtitle = (props: { state: NetworkTestState }) => {
  switch (props.state) {
    case NetworkTestState.NETWORK_GOOD:
      return (
        <div className="text-gray-500">
          Whist should run smoothly assuming you remain on the same network.
        </div>
      )
    default:
      return (
        <div className="text-gray-500">
          While you can still use Whist, you may experience stutters and
          latency. If possible, please move closer to your router or try again
          on different WiFi.
        </div>
      )
  }
}

const Text = (props: { state: NetworkTestState }) => {
  switch (props.state) {
    case NetworkTestState.IN_PROGRESS:
      return (
        <div className="text-gray-100 font-bold text-2xl mt-4 leading-10">
          Testing your Internet strength
        </div>
      )
    case NetworkTestState.NETWORK_GOOD:
      return (
        <div className="text-gray-100 font-bold text-2xl mt-4 leading-10">
          Your Internet is ready for Whist
        </div>
      )
    default:
      return (
        <div className="text-gray-100 font-bold text-2xl mt-4 leading-10">
          Your Internet is not strong enough
        </div>
      )
  }
}

const NetworkStats = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
    ping: number
  }
}) => {
  const stats = [
    {
      name: "Download Speed",
      stat: props.networkInfo?.downloadMbps ?? 0,
      units: "Mbps",
      required: MIN_ACCEPTABLE_DOWNLOAD_MBPS,
      operator: "greater",
    },
    {
      name: "Jitter",
      stat: props.networkInfo?.jitter ?? 0,
      units: "ms",
      required: MAX_ACCEPTABLE_JITTER_MS,
      operator: "less",
    },
    {
      name: "Ping",
      stat: props.networkInfo?.ping ?? 0,
      units: "ms",
      required: MAX_ACCEPTABLE_PING_MS,
      operator: "less",
    },
  ]

  return (
    <div>
      <dl className="grid grid-cols-1 rounded rounded-b-none bg-gray-800 overflow-hidden divide-gray-800 grid-cols-3 divide-y-0">
        {stats.map((item) => {
          const warning =
            item.operator === "greater"
              ? item.stat < item.required
              : item.stat > item.required
          return (
            <div key={item.name} className="px-4 py-2 sm:py-5 sm:p-6">
              <div
                className={classNames(
                  "w-2 h-2 rounded-full ml-2",
                  warning ? "bg-red-300" : "bg-green-300"
                )}
              ></div>
              <dt className="text-base font-normal text-gray-300 text-left pl-2 mt-3">
                <div className="text-xs sm:text-md">{item.name}</div>
              </dt>
              <dd className="mt-1 flex items-baseline pl-2">
                <div className="items-baseline text-lg sm:text-2xl font-bold justify-between text-gray-300">
                  {item.stat}
                </div>
                <div className="text-gray-500 text-xs ml-1">{item.units}</div>
              </dd>
            </div>
          )
        })}
      </dl>
    </div>
  )
}

const Button = (props: { state: NetworkTestState; onSubmit: () => void }) => {
  switch (props.state) {
    case NetworkTestState.IN_PROGRESS:
      return (
        <WhistButton
          contents="Continue"
          state={WhistButtonState.DISABLED}
          className="w-96 text-gray-900 bg-blue-light-light bg-opacity-20"
          onClick={props.onSubmit}
        />
      )
    case NetworkTestState.NETWORK_GOOD:
      return (
        <WhistButton
          contents="Continue"
          state={WhistButtonState.DEFAULT}
          className="w-96 bg-blue-light text-gray-800"
          onClick={props.onSubmit}
        />
      )
    default:
      return (
        <WhistButton
          contents="Continue Anyway"
          state={WhistButtonState.DEFAULT}
          className="w-96 bg-blue-light text-gray-800"
          onClick={props.onSubmit}
        />
      )
  }
}

const Network = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
    ping: number
  }
  onSubmit?: () => void
  withText?: boolean
  withButton?: boolean
}) => {
  const testState = getNetworkTestState(props.networkInfo)

  return (
    <div className="w-full max-w-md m-auto font-body text-center">
      {(props.withText ?? false) && (
        <>
          <div
            className="mt-6 animate-fade-in-up opacity-0"
            style={{ animationDelay: "400ms" }}
          >
            <Text state={testState} />
          </div>
          <div
            className="mt-2 animate-fade-in-up opacity-0"
            style={{ animationDelay: "400ms" }}
          >
            <Subtitle state={testState} />
          </div>
        </>
      )}
      <div
        className="mt-6 animate-fade-in-up opacity-0"
        style={{ animationDelay: "800ms" }}
      >
        <NetworkStats networkInfo={props.networkInfo} />
        <Progress
          percent={Math.min(props.networkInfo?.progress ?? 1, 100)}
          className="rounded-t-none h-1"
        />
      </div>
      {props.onSubmit !== undefined && (props.withButton ?? false) && (
        <div
          className="mt-6 h-12 animate-fade-in-up opacity-0"
          style={{ animationDelay: "800ms" }}
        >
          <Button state={testState} onSubmit={props.onSubmit} />
        </div>
      )}
    </div>
  )
}

export { Network }
