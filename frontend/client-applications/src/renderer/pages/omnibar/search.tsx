import React, { useEffect, useRef } from "react"

import { withContext } from "@app/renderer/context/omnibar"

const Search = () => {
  const context = withContext()

  const onChange = (e: any) => {
    context.setSearch(e.target.value)
  }

  const inputRef = useRef<HTMLInputElement>(null)

  useEffect(() => {
    // @ts-expect-error
    const ipc = window.ipcRenderer

    const listener = () => {
      inputRef.current?.focus()
    }
    ipc.on("window-focus", listener)

    // destructor
    return () => {
      ipc.removeListener?.("window-focus", listener)
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
        className="block w-full text-md rounded-md bg-transparent pt-1 focus:outline-none placeholder-gray-600"
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
