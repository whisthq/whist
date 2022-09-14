import * as React from "react"
import { useState } from "react"
import classNames from "classnames"
import { Switch } from "@headlessui/react"

const Checkbox = (props: {
  label: string
  default: boolean
  onChange: (checked: boolean) => void
}) => {
  const [checked, setChecked] = useState(props.default)

  const onChange = () => {
    const _checked = !checked
    setChecked(_checked)
    props.onChange(_checked)
  }

  return (
    <div className="w-full rounded rounded-t-none relative flex items-start space-x-6 py-4">
      <div className="flex-grow flex space-x-3">
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-5 w-5 text-gray-300 dark:text-gray-500"
          viewBox="0 0 20 20"
          fill="currentColor"
        >
          <path d="M5 4a1 1 0 00-2 0v7.268a2 2 0 000 3.464V16a1 1 0 102 0v-1.268a2 2 0 000-3.464V4zM11 4a1 1 0 10-2 0v1.268a2 2 0 000 3.464V16a1 1 0 102 0V8.732a2 2 0 000-3.464V4zM16 3a1 1 0 011 1v7.268a2 2 0 010 3.464V16a1 1 0 11-2 0v-1.268a2 2 0 010-3.464V4a1 1 0 011-1z" />
        </svg>
        <div className="font-medium text-gray-900 dark:text-gray-100 select-none text-sm">
          {props.label}
        </div>
      </div>
      <div className="ml-3 flex items-center h-5">
        <Switch
          checked={checked}
          onChange={onChange}
          className={classNames(
            "relative inline-flex flex-shrink-0 h-6 w-11 border-2 border-transparent rounded-full transition-colors ease-in-out duration-200 focus:outline-none focus:ring-2 focus:ring-offset-2 hover:outline-none",
            checked ? "bg-blue ring-blue" : "bg-gray-300 dark:bg-gray-600",
            "hover:ring-2 hover:ring-offset-2 hover:ring-blue cursor-pointer"
          )}
        >
          <span
            aria-hidden="true"
            className={classNames(
              checked ? "translate-x-5" : "translate-x-0",
              "pointer-events-none inline-block h-5 w-5 rounded-full bg-white shadow transform ring-0 transition ease-in-out duration-200"
            )}
          />
        </Switch>
      </div>
    </div>
  )
}

export default Checkbox
