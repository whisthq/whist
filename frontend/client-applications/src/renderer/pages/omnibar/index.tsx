import React, { useState } from "react"
import Search from "@app/renderer/pages/omnibar/search"
import Option from "@app/renderer/pages/omnibar/option"
import { Dollar, Logout, Mail, Signal } from "@app/renderer/pages/omnibar/icons"

import { useMainState } from "@app/utils/ipc"
import { WhistTrigger } from "@app/constants/triggers"

const Omnibar = () => {
  const [activeIndex, setActiveIndex] = useState(0)
  const [, setMainState] = useMainState()

  const options = [
    {
      icon: Dollar,
      text: "Payment and Billing",
      onClick: () =>
        setMainState({
          trigger: { name: WhistTrigger.showPaymentWindow, payload: undefined },
        }),
    },
    {
      icon: Logout,
      text: "Log Out",
      onClick: () =>
        setMainState({
          trigger: { name: WhistTrigger.showSignoutWindow, payload: undefined },
        }),
    },
    {
      icon: Mail,
      text: "Contact Support",
      onClick: () =>
        setMainState({
          trigger: { name: WhistTrigger.showSupportWindow, payload: undefined },
        }),
    },
    {
      icon: Signal,
      text: "Test My Internet Speed",
      onClick: () =>
        setMainState({
          trigger: {
            name: WhistTrigger.showSpeedtestWindow,
            payload: undefined,
          },
        }),
    },
  ]

  return (
    <div
      tabIndex={0}
      className="w-screen h-screen text-gray-100 text-center bg-gray-800 p-3 relative font-body focus:outline-none py-6 px-8 overflow-hidden"
    >
      <Search />
      <div className="overflow-y-scroll h-full">
        {options.map((option, index) => (
          <div onMouseEnter={() => setActiveIndex(index)}>
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
