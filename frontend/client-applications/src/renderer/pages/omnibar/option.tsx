import React from "react"
import className from "classnames"

const Option = (props: {
  icon: JSX.Element
  text: string
  active: boolean
  onClick: () => void
  rightElement?: JSX.Element
}) => {
  return (
    <div
      className={className(
        "flex py-4 px-6 cursor-pointer rounded justify-between w-full",
        props.active && "bg-blue"
      )}
      onClick={props.onClick}
    >
      <div className="flex space-x-6">
        <div
          className={className(
            "relative",
            props.active ? "text-gray-100" : "text-gray-400"
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
      {props.rightElement !== undefined && (
        <div className="justify-right">{props.rightElement}</div>
      )}
    </div>
  )
}

export default Option
