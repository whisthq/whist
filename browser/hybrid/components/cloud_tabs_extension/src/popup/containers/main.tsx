import * as React from "react"

import Host from "../components/host"
import Toggle from "../components/toggle"
import Checkbox from "../components/checkbox"
import Metrics from "../components/metrics"
import Network from "../components/network"

const Main = (props: {
  isWaiting: boolean
  isSaved: boolean
  networkInfo: any
  numberCloudTabs: number
  error?: string
  host?: string
  isCloudTab?: boolean
  onToggled: (toggled: boolean) => void
  onSaved: (saved: boolean) => void
}) => {
  const {
    host,
    isCloudTab,
    isWaiting,
    error,
    isSaved,
    onToggled,
    onSaved,
    numberCloudTabs,
    networkInfo,
  } = props

  return (
    <div className="w-screen bg-gray-100 dark:bg-gray-800">
      <div className="bg-white dark:bg-gray-900 px-6">
        {host !== undefined && (
          <>
            <Host host={host} />
            <div className="w-full h-px bg-gray-100 dark:bg-gray-800"></div>
          </>
        )}
        {isCloudTab !== undefined && (
          <>
            <Toggle
              initial={isCloudTab}
              onToggled={onToggled}
              isWaiting={isWaiting}
              error={error}
            />
            {numberCloudTabs > 0 && host !== undefined && error === undefined && (
              <div className="mt-2">
                <Checkbox
                  default={isSaved}
                  onChange={onSaved}
                  label={`Always stream ${host}`}
                />
              </div>
            )}
          </>
        )}
        {numberCloudTabs > 0 && (
          <div className="pb-4">
            <div className="w-full h-px bg-gray-100 dark:bg-gray-800 mb-4"></div>
            <Metrics numberCloudTabs={numberCloudTabs} />
          </div>
        )}
        <div className="pb-4">
          <div className="w-full h-px bg-gray-100 dark:bg-gray-800 mb-4"></div>
          <Network networkInfo={networkInfo} />
        </div>
      </div>
    </div>
  )
}

export default Main
