import React, { useState, useEffect } from "react"
import className from "classnames"

const Toggle = (props: {
  onChecked: (checked: boolean) => void
  default: boolean
}) => {
  const [checked, setChecked] = useState(props.default)

  const onClick = () => {
    setChecked(!checked)
    props.onChecked(!checked)
  }

  useEffect(() => {
    setChecked(props.default)
  }, [props.default])

  if (props.default === undefined) return <></>

  return (
    <div className="relative" onClick={onClick}>
      <input type="checkbox" className="sr-only" />
      <div
        className={className(
          "block w-12 h-6 rounded-full duration-100",
          checked ? "bg-blue-light" : "bg-gray-600"
        )}
      ></div>
      <div
        className={className(
          "dot absolute top-1 w-4 h-4 rounded-full transition duration-100",
          checked ? "right-1 bg-gray-700" : "left-1 bg-white"
        )}
      ></div>
    </div>
  )
}

export default Toggle
