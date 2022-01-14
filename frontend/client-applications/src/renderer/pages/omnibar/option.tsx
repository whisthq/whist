import React from "react"
import className from "classnames"

const Option = (props: {
  icon: JSX.Element
  text: string
  active: boolean
  onClick: () => void
}) => {
  return (
    <div
      className={className(
        "flex space-x-6 py-4 px-6 cursor-pointer rounded",
        props.active && "bg-blue"
      )}
      onClick={props.onClick}
    >
      <div
        className={className(
          "relative",
          props.active ? "text-gray-300" : "text-gray-600"
        )}
      >
        {props.icon}
      </div>
      <div
        className={className(
          "tracking-normal text-md",
          props.active ? "text-gray-100" : "text-gray-400"
        )}
      >
        {props.text}
      </div>
    </div>
  )
}

export default Option
