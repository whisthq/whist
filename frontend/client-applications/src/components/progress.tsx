import React, { useState, useEffect } from "react"
import classNames from "classnames"

export const Progress = (props: {
  percent: number
  text?: string
  className?: string
  mockProgress?: boolean
  mockSpeed?: number
}) => {
  const [progress, setProgress] = useState(1)

  useEffect(() => {
    const mockSpeed = props.mockSpeed ?? 1

    let _progress = progress

    const interval = setInterval(() => {
      if (_progress >= 96) clearInterval(interval)

      if (_progress <= 80) {
        _progress += 0.02 * mockSpeed
      } else {
        _progress += 0.01 * mockSpeed
      }

      setProgress(_progress)
    }, 0.1)
  }, [])

  return (
    <div className="relative border-none">
      <div
        className={classNames(
          `overflow-hidden mb-4 text-xs flex rounded bg-gray-800`,
          props.className ?? ""
        )}
      >
        <div
          style={{
            width:
              props.mockProgress ?? false
                ? `${progress.toString()}%`
                : `${props.percent.toString()}%`,
          }}
          className={`shadow-none flex flex-col text-center whitespace-nowrap text-white justify-center bg-gradient transition-width duration-150 ease`}
        ></div>
      </div>
    </div>
  )
}
