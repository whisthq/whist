import React, { useState, useEffect } from "react"
import { Progress } from "@app/components/progress"

import { useMainState } from "@app/renderer/utils/ipc"

const Update = () => {
  const [mainState] = useMainState()

  const [percentageDownloaded, setPercentageDownloaded] = useState(1)
  const [downloadSpeed, setDownloadSpeed] = useState(0)
  const [downloadedSize, setDownloadedSize] = useState(0)
  const [totalDownloadSize, setTotalDownloadSize] = useState(0)

  const sanitizeBytes = (fl: number) => Math.round((fl / 1000000) * 100) / 100

  useEffect(() => {
    if ((mainState.updateInfo ?? "") === "") return

    const updateInfo = JSON.parse(mainState.updateInfo)

    if (Number(updateInfo.percent) < percentageDownloaded) return

    setDownloadSpeed(sanitizeBytes(updateInfo.bytesPerSecond))
    setPercentageDownloaded(Number(updateInfo.percent))
    setDownloadedSize(sanitizeBytes(updateInfo.transferred))
    setTotalDownloadSize(sanitizeBytes(updateInfo.total))
  }, [mainState])

  return (
    <div className="flex flex-col justify-center items-center bg-gray-800 h-screen text-center draggable">
      <div className="w-full max-w-xs m-auto font-body">
        <div className="font-body text-xl font-semibold text-gray-300">
          Please wait while we download the latest version of Whist
        </div>
        <div className="text-sm text-gray-400 mt-1">
          Downloaded {downloadedSize.toString()} MB /{" "}
          {totalDownloadSize.toString()} MB at {downloadSpeed.toString()} Mbps
        </div>
        <div className="mt-6">
          <Progress percent={percentageDownloaded} className="h-2" />
        </div>
      </div>
    </div>
  )
}

export default Update
