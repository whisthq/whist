import classNames from "classnames"
import React, { useEffect, useState } from "react"

import Spinner from "@app/components/spinner"

const Dropdown = (props: {
  options: string[]
  onSubmit: (value: string) => void
  defaultText?: string
}) => {
  const [value, setValue] = useState(
    props.options.length > 0 ? props.options[0] : undefined
  )
  const [loading, setLoading] = useState(false)

  const onSubmit = (evt: any) => {
    if (value !== undefined) {
      setLoading(true)
      props.onSubmit(value)
      evt.preventDefault()
    }
  }

  useEffect(() => {
    if (value === undefined && props.options.length > 0)
      setValue(props.options[0])
  }, [props.options])

  return (
    <form onSubmit={onSubmit}>
      <div className="bg-gray-800 px-4 py-2 rounded w-full m-auto">
        {props.options.length === 0 ? (
          <div className="bg-gray-800 font-bold text-gray-300 outline-none mt-1 block w-full py-2 text-base border-gray-300 sm:text-sm">
            {props.defaultText}
          </div>
        ) : (
          <select
            className="bg-gray-800 font-bold text-gray-300 outline-none mt-1 block w-full py-2 text-base border-gray-300 sm:text-sm"
            defaultValue={props.options[0]}
            onChange={(evt: any) => {
              setValue(evt.target.value)
            }}
            value={value}
          >
            {props.options.map((option: string, index: number) => (
              <option key={index} value={option}>
                {option}
              </option>
            ))}
          </select>
        )}
      </div>
      <div className={classNames(value === undefined ? "opacity-40" : "")}>
        {loading ? (
          <div className="mt-4 px-12 w-96 mx-auto py-4 text-gray-300 text-gray-900 bg-blue-light rounded py-4 font-bold cursor-pointer">
            <div className="w-5 h-5 mx-auto text-gray-300">
              <Spinner />
            </div>
          </div>
        ) : (
          <>
            <input
              type="submit"
              value="Continue"
              className="mt-4 px-12 w-96 mx-auto py-4 text-gray-300 text-gray-900 bg-blue-light rounded py-4 font-bold cursor-pointer"
            />
          </>
        )}
      </div>
    </form>
  )
}

export default Dropdown
