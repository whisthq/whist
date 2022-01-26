import React, { useState, useEffect } from "react"

import { Progress } from "@app/components/progress"

const Importing = () => {
  const [progress, setProgress] = useState(1)

  useEffect(() => {
    let _progress = progress

    const interval = setInterval(() => {
      if (_progress >= 96) clearInterval(interval)

      if (_progress <= 80) {
        _progress += 0.02
      } else {
        _progress += 0.01
      }

      setProgress(_progress)
    }, 0.1)
  }, [])

  return (
    <div className="flex flex-col justify-center items-center bg-gray-900 h-screen text-center">
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="w-full max-w-xs m-auto font-body">
        <div className="font-body text-xl font-semibold text-gray-300 mt-5">
          Setting up your browser
        </div>
        <div className="text-sm text-gray-400 mt-1">
          This could take up to 20 seconds
        </div>
        <div className="mt-6">
          <Progress percent={progress} className="h-2" />
        </div>
      </div>
    </div>
  )
}

export default Importing
