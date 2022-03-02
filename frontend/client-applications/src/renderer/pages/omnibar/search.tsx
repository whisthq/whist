import React, { useRef } from "react"

import { withContext } from "@app/renderer/context/omnibar"
import { onUIEvent } from "@app/renderer/utils/ipc"
import { UIEvent } from "@app/@types/uiEvent"

const Search = () => {
  const context = withContext()

  const onChange = (e: any) => {
    context.setSearch(e.target.value)
  }

  const inputRef = useRef<HTMLInputElement>(null)

  onUIEvent((event) => {
    if (event === UIEvent.WINDOW_FOCUS) {
      // When the omnibar shows up, focus the search bar
      // and select any existing text (so the user can just
      // start typing to overwrite)
      inputRef.current?.focus()
      inputRef.current?.select()
    }
  })

  return (
    <div className="relative mb-6">
      <div className="absolute inset-y-0 right-0 flex items-center pointer-events-none space-x-1">
        <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
          ⌘
        </kbd>
        <kbd className="inline-flex items-center rounded px-2 py-1 text-xs font-medium text-gray-400 bg-gray-700">
          J
        </kbd>
      </div>
      <input
        className="block w-full text-md bg-transparent p-1 focus:outline-none placeholder-gray-600"
        autoFocus={true}
        placeholder="Search Whist Commands"
        value={context.search}
        onChange={onChange}
        ref={inputRef}
      />
    </div>
  )
}

export default Search
