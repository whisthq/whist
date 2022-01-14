import React, { useState, useEffect } from "react"
import FuzzySearch from "fuzzy-search"

import Search from "@app/renderer/pages/omnibar/search"
import Option from "@app/renderer/pages/omnibar/option"
import { Dollar, Logout, Mail, Signal } from "@app/renderer/pages/omnibar/icons"

import { useMainState } from "@app/utils/ipc"
import { WhistTrigger } from "@app/constants/triggers"
import { withContext } from "@app/renderer/pages/omnibar/context"

const Omnibar = () => {
  const [activeIndex, setActiveIndex] = useState(0)
  const [, setMainState] = useMainState()
  const context = withContext()

  const options = [
    {
      icon: Dollar,
      text: "Manage My Subscription",
      keywords: ["Billing", "Payment"],
      onClick: () =>
        setMainState({
          trigger: { name: WhistTrigger.showPaymentWindow, payload: undefined },
        }),
    },
    {
      icon: Logout,
      text: "Log Out",
      keywords: ["Signout", "Sign Out", "Exit", "Quit"],
      onClick: () =>
        setMainState({
          trigger: { name: WhistTrigger.showSignoutWindow, payload: undefined },
        }),
    },
    {
      icon: Mail,
      text: "Contact Support",
      keywords: ["Help", "Feedback", "Report Bug"],
      onClick: () =>
        setMainState({
          trigger: { name: WhistTrigger.showSupportWindow, payload: undefined },
        }),
    },
    {
      icon: Signal,
      text: "Test My Internet Speed",
      keywords: ["Connection"],
      onClick: () =>
        setMainState({
          trigger: {
            name: WhistTrigger.showSpeedtestWindow,
            payload: undefined,
          },
        }),
    },
  ]

  const searcher = new FuzzySearch(options, ["text", "keywords"], {
    caseSensitive: false,
  })
  const [filteredOptions, setFilteredOptions] = useState(options)

  useEffect(() => {
    setFilteredOptions(searcher.search(context.search))
  }, [context.search])

  return (
    <div
      tabIndex={0}
      className="w-screen h-screen text-gray-100 text-center bg-gray-800 p-3 relative font-body focus:outline-none py-6 px-8 overflow-hidden"
    >
      <Search />
      <div className="overflow-y-scroll h-full">
        {filteredOptions.map((option, index) => (
          <div onMouseEnter={() => setActiveIndex(index)} key={index}>
            <Option
              icon={option.icon()}
              text={option.text}
              active={activeIndex === index}
              onClick={option.onClick}
            />
          </div>
        ))}
      </div>
    </div>
  )
}

export default Omnibar
