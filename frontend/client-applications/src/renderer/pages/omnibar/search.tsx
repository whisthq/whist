import React from "react"

import { withContext } from "@app/renderer/pages/omnibar/context"

const Search = () => {
  const context = withContext()

  const onChange = (e: any) => {
    context.setSearch(e.target.value)
  }

  return (
    <div className="relative mb-6">
      <div className="absolute inset-y-0 right-0 flex items-center pointer-events-none space-x-2">
        <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
          Option
        </kbd>
        <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
          W
        </kbd>
      </div>
      <input
        className="block w-full text-md rounded-md bg-transparent pt-1 focus:outline-none placeholder-gray-600"
        autoFocus
        placeholder="Search Whist Commands"
        value={context.search}
        onChange={onChange}
      />
    </div>
  )
}

export default Search
