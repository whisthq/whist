import React, { useState, useEffect } from "react"
import FuzzySearch from "fuzzy-search"

import Search from "@app/renderer/pages/omnibar/search"
import Option from "@app/renderer/pages/omnibar/option"
import {
  Dollar,
  Logout,
  Mail,
  Signal,
  Globe,
  Rewind,
  Document,
} from "@app/renderer/pages/omnibar/icons"

import { useMainState } from "@app/renderer/utils/ipc"
import { WhistTrigger } from "@app/constants/triggers"
import { withContext } from "@app/renderer/pages/omnibar/context"
import Toggle from "@app/components/toggle"
import { StateIPC } from "@app/@types/state"

const createOptions = (mainState: StateIPC, setMainState: any) => [
  {
    icon: Globe,
    text: "Make Whist My Default Browser",
    keywords: ["Set Default", "Set As Default"],
    rightElement: (
      <Toggle
        onChecked={(checked: boolean) => {
          setMainState({
            trigger: {
              name: WhistTrigger.setDefaultBrowser,
              payload: { default: checked },
            },
          })
        }}
        default={mainState.isDefaultBrowser}
      />
    ),
    onClick: () => {},
  },
  {
    icon: Rewind,
    text: "Restore Tabs on Launch",
    keywords: [""],
    rightElement: (
      <Toggle
        onChecked={(checked: boolean) => {
          setMainState({
            trigger: {
              name: WhistTrigger.restoreLastSession,
              payload: { restore: checked },
            },
          })
        }}
        default={mainState.restoreLastSession}
      />
    ),
    onClick: () => {},
  },
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
  {
    icon: Document,
    text: "View Open Source Licenses",
    keywords: ["About"],
    onClick: () =>
      setMainState({
        trigger: {
          name: WhistTrigger.showLicenseWindow,
          payload: undefined,
        },
      }),
  },
]

const Omnibar = () => {
  const [activeIndex, setActiveIndex] = useState(0)
  const [mainState, setMainState] = useMainState()
  const context = withContext()

  const [options, setOptions] = useState(createOptions(mainState, setMainState))

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
      tabIndex={0}
      className="w-screen h-screen text-gray-100 text-center bg-gray-800 p-3 relative font-body focus:outline-none py-6 px-8 overflow-hidden"
    >
      <Search />
      <div className="overflow-y-scroll h-full pb-12">
        {options.map((option, index) => (
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
