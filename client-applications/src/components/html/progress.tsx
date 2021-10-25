import React from "react"

export const Progress = (props: {
  percent: number
  color: string
  text?: string
}) => (
  <div className="relative pt-1">
    {props.text && (
      <div className="flex mb-2 items-center justify-between">
        <div>
          <span
            className={`text-xs font-semibold inline-block py-1 px-2 uppercase rounded-full text-${props.color}-600 bg-${props.color}-200`}
          >
            {props.text}
          </span>
        </div>
        <div className="text-right">
          <span className="text-xs font-semibold inline-block text-pink-600">
            {props.percent.toString()}%
          </span>
        </div>
      </div>
    )}
    <div
      className={`overflow-hidden h-2 mb-4 text-xs flex rounded bg-${props.color}-200`}
    >
      <div
        style={{ width: `${props.percent.toString()}%` }}
        className={`shadow-none flex flex-col text-center whitespace-nowrap text-white justify-center bg-${props.color}-500`}
      ></div>
    </div>
  </div>
)
