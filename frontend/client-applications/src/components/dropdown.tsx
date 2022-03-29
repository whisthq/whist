import React, { useState } from "react"

const Dropdown = (props: {
  options: string[]
  onSubmit: (value: string) => void
  submitButton: JSX.Element
}) => {
  const [value, setValue] = useState(props.options?.[0])

  const onSubmit = (evt: any) => {
    props.onSubmit(value)
    evt.preventDefault()
  }

  return (
    <form onSubmit={onSubmit}>
      <div className="bg-gray-800 px-4 py-2 rounded w-full m-auto">
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
      </div>
      {props.submitButton}
    </form>
  )
}

export default Dropdown
