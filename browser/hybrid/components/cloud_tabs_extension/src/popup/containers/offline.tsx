import * as React from "react"

import Toggle from "../components/toggle"

const Offline = () => {
  return (
    <div className="w-screen bg-white dark:bg-gray-900">
      <div className="px-6">
        <Toggle
          initial={false}
          onToggled={() => {}}
          isWaiting={false}
          error={
            "Whist is having trouble finding your network. Please check your Internet to see if you're connected."
          }
        />
      </div>
    </div>
  )
}

export default Offline
