import React from "react"

const Search = () => {
  return (
    <div className="relative mb-6">
      <div className="absolute inset-y-0 right-0 flex items-center pointer-events-none space-x-2">
        <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
          Option
        </kbd>
        <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
          A
        </kbd>
      </div>
      <input
        className="block w-full text-md rounded-md bg-transparent pt-1 focus:outline-none placeholder-gray-600"
        autoFocus
        placeholder="Search Whist Commands"
        // value={context.userInput}
        // onKeyDown={onKeyDown}
        // onChange={onChange}
      />
    </div>
  )
}

export default Search
