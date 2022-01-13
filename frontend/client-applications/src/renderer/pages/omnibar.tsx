import React from "react"

const Search = () => {
  return (
    <div className="relative py-4 border border-gray-600 border-t-0 border-l-0 border-r-0">
      <div className="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none bg-none">
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-4 w-4 text-gray-500"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"
          />
        </svg>
      </div>
      <input
        className="block w-full pl-10 sm:text-sm rounded-md bg-transparent pt-1 focus:outline-none"
        autoFocus
        placeholder=""
        // value={context.userInput}
        // onKeyDown={onKeyDown}
        // onChange={onChange}
      />
    </div>
  )
}

const Omnibar = () => {
  return (
    <div
      tabIndex={0}
      className="w-screen h-screen text-gray-100 text-center bg-gray-900 bg-opacity-90 p-3 relative font-body focus:outline-none"
    >
      <Search />
    </div>
  )
}

export default Omnibar
