import React, { useState, useEffect } from "react"
import { Progress } from "@app/components/progress"

const Importing = () => {
  const [progress, setProgress] = useState(1)

  useEffect(() => {
    setInterval(() => {
      if (progress >= 99) clearInterval()
      setProgress(progress + 0.5)
    }, 200)
  }, [])

  return (
    <div className="flex flex-col justify-center items-center bg-gray-800 h-screen text-center">
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="w-full max-w-xs m-auto font-body">
        <div className="font-body text-xl font-semibold text-gray-300">
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
