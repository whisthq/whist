import React, { useState, useEffect } from "react"
import { Line } from "rc-progress"

import { useMainState } from "@app/utils/ipc"

import { Logo } from "@app/components/html/logo"

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
        <div className="flex flex-col justify-center items-center bg-white h-screen text-center">
            <div className="w-full max-w-xs m-auto font-body">
                <Logo />
                <div className="font-body mt-8 text-xl font-semibold">
                    An update is downloading
                </div>
                <div className="text-sm">
                    Downloaded {downloadedSize.toString()} MB /{" "}
                    {totalDownloadSize.toString()} MB at{" "}
                    {downloadSpeed.toString()} Mbps
                </div>
                <div className="mt-6">
                    <Line
                        percent={percentageDownloaded}
                        strokeWidth={3}
                        trailWidth={3}
                        strokeColor="#00FFA2"
                    />
                </div>
            </div>
        </div>
    )
}

export default Update
