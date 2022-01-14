import React from "react"
import classNames from "classnames"

import { Progress } from "@app/components/progress"
import { WhistButton, WhistButtonState } from "@app/components/button"

const MAX_ACCEPTABLE_JITTER_MS = 20
const MIN_ACCEPTABLE_DOWNLAOD_MBPS = 50

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
      }
    | undefined
) => {
  // Network test is in progress
  if (networkInfo === undefined || networkInfo.progress < 100)
    return NetworkTestState.IN_PROGRESS

  // Network test is finished and the network is good
  if (
    networkInfo.progress >= 100 &&
    networkInfo.downloadMbps >= MIN_ACCEPTABLE_DOWNLAOD_MBPS &&
    networkInfo.jitter < MAX_ACCEPTABLE_JITTER_MS
  )
    return NetworkTestState.NETWORK_GOOD

  // Network may not be good
  return NetworkTestState.NETWORK_NOT_GOOD
}

const Icon = (props: { state: NetworkTestState }) => {
  switch (props.state) {
    case NetworkTestState.IN_PROGRESS:
      return (
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-12 w-12 text-mint m-auto"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0M1.394 9.393c5.857-5.857 15.355-5.857 21.213 0"
          />
        </svg>
      )
    case NetworkTestState.NETWORK_GOOD:
      return (
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-12 w-12 text-mint m-auto"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z"
          />
        </svg>
      )
    default:
      return (
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-12 w-12 text-red-500 m-auto"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M20.618 5.984A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016zM12 9v2m0 4h.01"
          />
        </svg>
      )
  }
}

const Text = (props: { state: NetworkTestState }) => {
  switch (props.state) {
    case NetworkTestState.IN_PROGRESS:
      return (
        <div className="text-gray-300 font-semibold text-xl">
          Testing your Internet strength
        </div>
      )
    case NetworkTestState.NETWORK_GOOD:
      return (
        <div className="text-gray-300 font-semibold text-xl">
          Your Internet is strong enough for Whist
        </div>
      )
    default:
      return (
        <div className="text-gray-300 font-semibold text-xl">
          Your Internet may not be strong enough
        </div>
      )
  }
}

const NetworkStats = (props: {
  networkInfo: {
    jitter: number
    progress: number
    downloadMbps: number
  }
}) => {
  const stats = [
    {
      name: "Download Speed",
      stat: props.networkInfo?.downloadMbps ?? 0,
      units: "Mbps",
      required: MIN_ACCEPTABLE_DOWNLAOD_MBPS,
      operator: "greater",
    },
    {
      name: "Jitter",
      stat: props.networkInfo?.jitter ?? 0,
      units: "ms",
      required: MAX_ACCEPTABLE_JITTER_MS,
      operator: "less",
    },
  ]

  return (
    <div>
      <dl className="grid grid-cols-1 rounded rounded-b-none bg-gray-900 overflow-hidden divide-gray-800 grid-cols-2 divide-y-0">
        {stats.map((item) => {
          const warning =
            item.operator === "greater"
              ? item.stat < item.required
              : item.stat > item.required
          return (
            <div key={item.name} className="px-4 py-2 sm:py-5 sm:p-6">
              <dt className="text-base font-normal text-gray-300 text-left">
                <div className="text-xs sm:text-md">{item.name}</div>
              </dt>
              <dd className="mt-1 flex items-baseline">
                <div
                  className={classNames(
                    "items-baseline text-lg sm:text-2xl font-semibold justify-between",
                    warning ? "text-red-500" : "text-mint"
                  )}
                >
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
      return <></>
    case NetworkTestState.NETWORK_GOOD:
      return (
        <WhistButton
          contents="Continue"
          state={WhistButtonState.DEFAULT}
          className="w-96 bg-mint text-gray-800"
          onClick={props.onSubmit}
        />
      )
    default:
      return (
        <WhistButton
          contents="Continue Anyway"
          state={WhistButtonState.DEFAULT}
          className="w-96 bg-mint text-gray-800"
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
  }
  onSubmit?: () => void
  withText?: boolean
}) => {
  const testState = getNetworkTestState(props.networkInfo)

  return (
    <div className="w-full max-w-sm m-auto font-body text-center">
      {(props.withText ?? false) && (
        <>
          <Icon state={testState} />
          <div className="mt-4 mb-8">
            <Text state={testState} />
          </div>
        </>
      )}
      <div>
        <NetworkStats networkInfo={props.networkInfo} />
      </div>
      <div>
        <Progress
          percent={Math.min(props.networkInfo?.progress ?? 1, 98)}
          className="rounded-t-none h-1"
        />
      </div>
      {props.onSubmit !== undefined && (
        <div className="mt-6 h-12">
          <Button state={testState} onSubmit={props.onSubmit} />
        </div>
      )}
    </div>
  )
}

export { Network }
