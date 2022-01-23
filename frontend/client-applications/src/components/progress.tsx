import React from "react"
import classNames from "classnames"

export const Progress = (props: {
  percent: number
  text?: string
  className?: string
}) => (
  <div className="relative border-none">
    <div
      className={classNames(
        `overflow-hidden mb-4 text-xs flex rounded`,
        props.className ?? ""
      )}
    >
      <div
        style={{ width: `${props.percent.toString()}%` }}
        className={`shadow-none flex flex-col text-center whitespace-nowrap text-white justify-center bg-gradient`}
      ></div>
    </div>
  </div>
)
