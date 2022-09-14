import React from "react"

const Metrics = (props: { numberCloudTabs: number }) => {
  const { numberCloudTabs } = props
  return (
    <div className="w-full">
      <div className="flex space-x-3">
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
        <div className="text-sm">
          <div className="font-semibold text-gray-900 dark:text-gray-100">
            {Math.min(0.6 * numberCloudTabs, 25).toFixed(1)} GB RAM saved
          </div>
        </div>
      </div>
    </div>
  )
}

export default Metrics
