import React from "react"

const Dropdown = (props: {
  options: string[]
  onSelect: (option: string) => void
}) => {
  return (
    <div>
      <div className="bg-gray-800 px-4 py-2 rounded w-full m-auto">
        <select
          className="bg-gray-800 font-bold text-gray-300 outline-none mt-1 block w-full py-2 text-base border-gray-300 sm:text-sm"
          defaultValue={props.options[0]}
          onChange={(evt: any) => {
            props.onSelect(evt.target.value)
          }}
        >
          {props.options.map((option: string, index: number) => (
            <option key={index} value={option}>
              {option}
            </option>
          ))}
        </select>
      </div>
    </div>
  )
}

export default Dropdown
