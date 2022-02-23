import React from "react"
import range from "lodash.range"

export default (props: {
  total: number
  current: number
  onClick: (index: number) => void
}) => {
  const _range = range(0, props.total)
  return (
    <nav className="flex items-center justify-center" aria-label="Progress">
      <p className="text-sm text-gray-400 font-medium">
        Step {props.current + 1} of {props.total}
      </p>
      <ol role="list" className="ml-8 flex items-center space-x-5">
        {_range.map((x) => (
          <li key={x}>
            {x < props.current ? (
              <div
                onClick={() => props.onClick(x)}
                className="block w-2.5 h-2.5 bg-indigo-600 rounded-full hover:bg-indigo-400 cursor-pointer"
              ></div>
            ) : x === props.current ? (
              <div
                onClick={() => props.onClick(x)}
                className="relative flex items-center justify-center"
                aria-current="step"
              >
                <span className="absolute w-5 h-5 p-px flex" aria-hidden="true">
                  <span className="w-full h-full rounded-full bg-indigo-600 bg-opacity-50" />
                </span>
                <span
                  className="relative block w-2.5 h-2.5 bg-indigo-600 rounded-full"
                  aria-hidden="true"
                />
              </div>
            ) : (
              <div className="block w-2.5 h-2.5 bg-indigo-400 rounded-full bg-opacity-20"></div>
            )}
          </li>
        ))}
      </ol>
    </nav>
  )
}
