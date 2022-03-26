import React from "react"

import { Progress } from "@app/components/progress"

const Importing = () => {
  return (
    <div className="flex flex-col justify-center items-center bg-gray-900 h-screen text-center">
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="w-full max-w-xs m-auto font-body">
        <div className="font-body text-2xl font-bold text-gray-300 mt-5">
          Importing your bookmarks and settings
        </div>
        <div className="text-sm text-gray-400 mt-1">
          This could take up to 20 seconds
        </div>
        <div className="mt-6">
          <Progress percent={0} className="h-2" mockProgress={true} />
        </div>
      </div>
    </div>
  )
}

export default Importing
