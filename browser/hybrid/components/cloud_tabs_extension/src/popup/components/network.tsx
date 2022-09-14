import React from "react"

const maxPingTime = 25

const Network = (props: { networkInfo: any }) => {
  const { networkInfo } = props

  const icon = () => {
    if (networkInfo === undefined)
      return (
        <div role="status">
          <svg
            className="inline mr-1 w-4 h-4 text-gray-200 animate-spin dark:text-gray-600 fill-blue"
            viewBox="0 0 100 101"
            fill="none"
            xmlns="http://www.w3.org/2000/svg"
          >
            <path
              d="M100 50.5908C100 78.2051 77.6142 100.591 50 100.591C22.3858 100.591 0 78.2051 0 50.5908C0 22.9766 22.3858 0.59082 50 0.59082C77.6142 0.59082 100 22.9766 100 50.5908ZM9.08144 50.5908C9.08144 73.1895 27.4013 91.5094 50 91.5094C72.5987 91.5094 90.9186 73.1895 90.9186 50.5908C90.9186 27.9921 72.5987 9.67226 50 9.67226C27.4013 9.67226 9.08144 27.9921 9.08144 50.5908Z"
              fill="currentColor"
            />
            <path
              d="M93.9676 39.0409C96.393 38.4038 97.8624 35.9116 97.0079 33.5539C95.2932 28.8227 92.871 24.3692 89.8167 20.348C85.8452 15.1192 80.8826 10.7238 75.2124 7.41289C69.5422 4.10194 63.2754 1.94025 56.7698 1.05124C51.7666 0.367541 46.6976 0.446843 41.7345 1.27873C39.2613 1.69328 37.813 4.19778 38.4501 6.62326C39.0873 9.04874 41.5694 10.4717 44.0505 10.1071C47.8511 9.54855 51.7191 9.52689 55.5402 10.0491C60.8642 10.7766 65.9928 12.5457 70.6331 15.2552C75.2735 17.9648 79.3347 21.5619 82.5849 25.841C84.9175 28.9121 86.7997 32.2913 88.1811 35.8758C89.083 38.2158 91.5421 39.6781 93.9676 39.0409Z"
              fill="currentFill"
            />
          </svg>
        </div>
      )

    if (networkInfo.pingTime < maxPingTime)
      return (
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-5 w-5 text-green-500"
          viewBox="0 0 20 20"
          fill="currentColor"
        >
          <path
            fillRule="evenodd"
            d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z"
            clipRule="evenodd"
          />
        </svg>
      )

    return (
      <svg
        xmlns="http://www.w3.org/2000/svg"
        viewBox="0 0 24 24"
        fill="currentColor"
        className="h-5 w-5 text-red-500"
      >
        <path
          fillRule="evenodd"
          d="M2.25 12c0-5.385 4.365-9.75 9.75-9.75s9.75 4.365 9.75 9.75-4.365 9.75-9.75 9.75S2.25 17.385 2.25 12zM12 8.25a.75.75 0 01.75.75v3.75a.75.75 0 01-1.5 0V9a.75.75 0 01.75-.75zm0 8.25a.75.75 0 100-1.5.75.75 0 000 1.5z"
          clipRule="evenodd"
        />
      </svg>
    )
  }

  const text = () => {
    if (networkInfo === undefined) return "Detecting your network latency..."
    if (networkInfo.pingTime < maxPingTime)
      return (
        <>
          <div className="font-semibold text-gray-900 dark:text-gray-100 pb-1.5 text-sm">
            Your Internet is strong
          </div>
          <div className="flex space-x-3 text-gray-600 dark:text-gray-300 text-xs">
            <div>Ping: {networkInfo.pingTime}ms</div>
            <div className="w-1.5 h-1.5 rounded-full bg-gray-400 dark:bg-gray-600 relative top-[0.3rem]"></div>
            <div>Closest Server: {networkInfo.region}</div>
          </div>
        </>
      )

    return (
      <>
        <div className="font-semibold text-gray-900 dark:text-gray-100 pb-1.5 text-sm">
          Your ping is high
        </div>
        <div className="flex space-x-3 text-gray-600 dark:text-gray-300 text-xs">
          <div>Ping: {networkInfo.pingTime}ms</div>
          <div className="w-1.5 h-1.5 rounded-full bg-gray-400 dark:bg-gray-600 relative top-[0.3rem]"></div>
          <div>Closest Server: {networkInfo.region}</div>
        </div>
      </>
    )
  }

  return (
    <div className="w-full">
      <div className="flex space-x-3">
        <div>{icon()}</div>
        <div>{text()}</div>
      </div>
    </div>
  )
}

export default Network
