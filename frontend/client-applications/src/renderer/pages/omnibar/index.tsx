import React, { useState, useEffect } from "react"
import FuzzySearch from "fuzzy-search"

import Search from "@app/renderer/pages/omnibar/search"
import Option from "@app/renderer/pages/omnibar/option"

import { useMainState } from "@app/renderer/utils/ipc"
import { withContext } from "@app/renderer/context/omnibar"
import { createOptions } from "@app/renderer/utils/omnibar"

const Omnibar = () => {
  const [activeIndex, setActiveIndex] = useState(0)
  const [mainState, setMainState] = useMainState()
  const [options, setOptions] = useState(createOptions(mainState, setMainState))
  const context = withContext()

  const onKeyDown = (e: any) => {
    if (e.key === "Enter") options[activeIndex].onClick()
    if (e.key === "ArrowDown")
      setActiveIndex(Math.min(activeIndex + 1, options.length))
    if (e.key === "ArrowUp") setActiveIndex(Math.max(0, activeIndex - 1))
  }

  const startingID = () => {
    if (activeIndex < 5) return 0
    if (activeIndex > options.length - 5) return options.length - 5
    return activeIndex
  }

  useEffect(() => {
    const searcher = new FuzzySearch(
      createOptions(mainState, setMainState),
      ["text", "keywords"],
      {
        caseSensitive: false,
      }
    )
    setOptions(searcher.search(context.search))
  }, [context.search, mainState])

  return (
    <div
      onKeyDown={onKeyDown}
      tabIndex={0}
      className="w-screen h-screen text-gray-100 text-center bg-gray-800 p-3 relative font-body focus:outline-none py-6 px-8 overflow-hidden"
    >
      <Search />
      <div className="overflow-y-scroll h-full pb-12">
        {options.slice(startingID()).map((option, index) => (
          <div onMouseEnter={() => setActiveIndex(index)} key={index}>
            <Option
              icon={option.icon()}
              text={option.text}
              active={activeIndex === index}
              onClick={option.onClick}
              rightElement={option?.rightElement}
            />
          </div>
        ))}
      </div>
    </div>
  )
}

export default Omnibar
