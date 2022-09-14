import * as React from "react"
import classNames from "classnames"

import { useState } from "react"
import { Switch } from "@headlessui/react"

import CloudOff from "../assets/cloud-off.png"
import CloudOn from "../assets/cloud-on.png"
import CloudWaiting from "../assets/cloud-waiting.png"
import CloudWarning from "../assets/cloud-warning.png"

const Toggle = (props: {
  initial: boolean
  isWaiting: boolean
  error: string | undefined
  onToggled: (enabled: boolean) => void
}) => {
  const [enabled, setEnabled] = useState(props.initial || props.isWaiting)

  const onChange = () => {
    const _enabled = !enabled
    props.onToggled(_enabled)
    setEnabled(_enabled)
  }

  const text = () => {
    if (props.error !== undefined) return "This page shouldn't be streamed"
    if (props.isWaiting) return "Whist is connecting"
    if (enabled) return `Streaming is ON for this site`
    return `Streaming is OFF for this site`
  }

  const icon = () => {
    if (props.error !== undefined) return CloudWarning
    if (props.isWaiting) return CloudWaiting
    if (enabled) return CloudOn
    return CloudOff
  }

  const description = () => {
    if (props.error !== undefined) return props.error
    if (props.isWaiting)
      return "This should only take a few seconds. If it takes any longer, please report it as a bug so we can fix!"
    if (enabled)
      return "This site is being streamed. You can disable streaming if you experience latency due to poor Internet."
    return "Enable streaming to make this site faster and use much less memory."
  }

  return (
    <div className="py-4">
      <Switch.Group
        as="div"
        className="flex items-center justify-between rounded space-x-6 duration-100"
      >
        <span className="flex-grow flex flex-col space-y-1">
          <Switch.Label
            as="span"
            className="font-medium text-gray-900 dark:text-gray-100 flex space-x-2"
            passive
          >
            <div className="text-sm">{text()}</div>
          </Switch.Label>
        </span>
        <Switch
          disabled={props.error !== undefined}
          checked={enabled}
          onChange={onChange}
          className={classNames(
            "relative inline-flex flex-shrink-0 h-6 w-11 border-2 border-transparent rounded-full transition-colors ease-in-out duration-200 focus:outline-none focus:ring-2 focus:ring-offset-2 hover:outline-none",
            enabled ? "bg-blue ring-blue" : "bg-gray-300 dark:bg-gray-600",
            props.error === undefined
              ? "hover:ring-2 hover:ring-offset-2 hover:ring-blue cursor-pointer"
              : "cursor-default"
          )}
        >
          <span
            aria-hidden="true"
            className={classNames(
              enabled ? "translate-x-5" : "translate-x-0",
              "pointer-events-none inline-block h-5 w-5 rounded-full bg-white shadow transform ring-0 transition ease-in-out duration-200"
            )}
          />
        </Switch>
      </Switch.Group>
      <img className="mb-6 mt-8 mx-auto w-20 h-20" src={icon()} />
      <div className="text-center text-xs text-gray-700 dark:text-gray-300 px-2 tracking-wide">
        {description()}
      </div>
    </div>
  )
}

export default Toggle
