import React, { useState } from "react"
import classNames from "classnames"

const Popover = (props: {
  element: JSX.Element
  popup: string | JSX.Element
}) => {
  const [isHovering, setIsHovering] = useState(false)

  return (
    <div className="relative">
      <div
        className={classNames(
          "absolute p-4 bg-gray-800 rounded duration-150 bottom-16 left-2 text-gray-400 text-sm text-left",
          isHovering ? "opacity-100" : "opacity-0"
        )}
      >
        {props.popup}
      </div>
      <div
        onMouseEnter={() => setIsHovering(true)}
        onMouseLeave={() => setIsHovering(false)}
      >
        {props.element}
      </div>
    </div>
  )
}

export default Popover
