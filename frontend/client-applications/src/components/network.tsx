import React from "react"
import classNames from "classnames"

import Logo from "@app/components/icons/logo.svg"
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
          className="h-10 w-10 text-mint m-auto animate-pulse"
          viewBox="0 0 20 20"
          fill="currentColor"
        >
          <path
            fill-rule="evenodd"
            d="M5.05 3.636a1 1 0 010 1.414 7 7 0 000 9.9 1 1 0 11-1.414 1.414 9 9 0 010-12.728 1 1 0 011.414 0zm9.9 0a1 1 0 011.414 0 9 9 0 010 12.728 1 1 0 11-1.414-1.414 7 7 0 000-9.9 1 1 0 010-1.414zM7.879 6.464a1 1 0 010 1.414 3 3 0 000 4.243 1 1 0 11-1.415 1.414 5 5 0 010-7.07 1 1 0 011.415 0zm4.242 0a1 1 0 011.415 0 5 5 0 010 7.072 1 1 0 01-1.415-1.415 3 3 0 000-4.242 1 1 0 010-1.415zM10 9a1 1 0 011 1v.01a1 1 0 11-2 0V10a1 1 0 011-1z"
            clip-rule="evenodd"
          />
        </svg>
      )
    case NetworkTestState.NETWORK_GOOD:
      return (
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-10 w-10 text-mint m-auto"
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
          className="h-10 w-10 text-red-400 m-auto"
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
        <div className="text-gray-100 font-bold text-lg mt-4">
          Testing your Internet strength
        </div>
      )
    case NetworkTestState.NETWORK_GOOD:
      return (
        <div className="text-gray-100 font-bold text-lg mt-4">
          Your Internet is ready for Whist
        </div>
      )
    default:
      return (
        <div className="text-gray-100 font-bold text-lg mt-4">
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
      <dl className="grid grid-cols-1 rounded rounded-b-none bg-gray-800 overflow-hidden divide-gray-800 grid-cols-2 divide-y-0">
        {stats.map((item) => {
          const warning =
            item.operator === "greater"
              ? item.stat < item.required
              : item.stat > item.required
          return (
            <div key={item.name} className="px-4 py-2 sm:py-5 sm:p-6">
              <dt className="text-base font-normal text-gray-300 text-left pl-2">
                <div className="text-xs sm:text-md">{item.name}</div>
              </dt>
              <dd className="mt-1 flex items-baseline pl-2">
                <div
                  className={classNames(
                    "items-baseline text-lg sm:text-2xl font-semibold justify-between",
                    warning ? "text-red-400" : "text-mint"
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
      return (
        <WhistButton
          contents="Continue"
          state={WhistButtonState.DISABLED}
          className="w-96 text-gray-900 bg-mint-light bg-opacity-20"
          onClick={props.onSubmit}
        />
      )
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
  withButton?: boolean
}) => {
  const testState = getNetworkTestState(props.networkInfo)

  return (
    <div className="w-full max-w-sm m-auto font-body text-center">
      {(props.withText ?? false) && (
        <>
          <div className="animate-fade-in-up opacity-0">
            <Icon state={testState} />
          </div>
          <div
            className="mt-6 animate-fade-in-up opacity-0"
            style={{ animationDelay: "400ms" }}
          >
            <Text state={testState} />
          </div>
        </>
      )}
      <div
        className="mt-8 animate-fade-in-up opacity-0"
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
