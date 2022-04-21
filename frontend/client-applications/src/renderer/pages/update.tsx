import React, { useState, useEffect } from "react"
import { Progress } from "@app/components/progress"
import Logo from "@app/components/icons/logo.svg"

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

    if (updateInfo.bytesPerSecond !== undefined)
      setDownloadSpeed(sanitizeBytes(updateInfo.bytesPerSecond))
    if (updateInfo.percent !== undefined)
      setPercentageDownloaded(Number(updateInfo.percent))
    if (updateInfo.transferred !== undefined)
      setDownloadedSize(sanitizeBytes(updateInfo.transferred))
    if (updateInfo.total !== undefined)
      setTotalDownloadSize(sanitizeBytes(updateInfo.total))
  }, [mainState])

  return (
    <div className="flex flex-col h-screen w-full font-body bg-gray-900">
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="w-full text-center pt-16">
        <img
          src={Logo}
          className="w-24 h-24 mx-auto opacity-0 animate-fade-in-up"
        />
        <div
          className="mt-12 text-xl text-gray-300 font-bold opacity-0 animate-fade-in-up"
          style={{ animationDelay: "800ms" }}
        >
          Whist is updating
        </div>
      </div>
      <div
        className="mt-4 w-full max-w-xs mx-auto opacity-0 animate-fade-in-up"
        style={{ animationDelay: "1200ms" }}
      >
        <Progress percent={percentageDownloaded} className="h-2" />
      </div>
      <div
        className="w-full text-center opacity-0 animate-fade-in-up"
        style={{ animationDelay: "1600ms" }}
      >
        <div className="text-gray-500 text-sm">
          {" "}
          Downloaded {downloadedSize.toString()} MB /{" "}
          {totalDownloadSize.toString()} MB at {downloadSpeed.toString()} Mbps
        </div>
      </div>
      <div
        className="absolute bottom-4 left-0 right-0 w-full bg-gray-800 bg-opacity-50 px-8 py-4 text-center text-gray-400 text-sm max-w-md rounded m-auto opacity-0 animate-fade-in-up"
        style={{ animationDelay: "2000ms" }}
      >
        Please allow a minute while we download your update. Exciting new
        changes are ahead!
      </div>
    </div>
  )
}

export default Update
