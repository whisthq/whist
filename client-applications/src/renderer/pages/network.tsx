import React, { useState, useEffect } from "react"
import { Progress } from "@app/components/html/progress"

import { useMainState } from "@app/utils/ipc"

const Update = () => {
  const [mainState] = useMainState()

  return (
    <div className="flex flex-col justify-center items-center bg-black bg-opacity-90 h-screen text-center">
      <div className="w-full max-w-xs m-auto font-body">
        <div className="font-body text-xl font-semibold text-gray-300">
          Testing your Internet connection
        </div>
        <div className="mt-6">
          <Progress percent={10} color="blue" />
        </div>
      </div>
    </div>
  )
}

export default Update
