import React from "react"

import Globe from "@app/components/icons/globe"
import Document from "@app/components/icons/document"
import Dollar from "@app/components/icons/dollar"
import Logout from "@app/components/icons/logout"
import Rewind from "@app/components/icons/rewind"
import Signal from "@app/components/icons/signal"
import Download from "@app/components/icons/download"
import Toggle from "@app/components/toggle"

import { StateIPC } from "@app/@types/state"
import { WhistTrigger } from "@app/constants/triggers"

const createOptions = (mainState: StateIPC, setMainState: any) => {
  return [
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
      icon: Download,
      text: "Import Bookmarks and Settings",
      keywords: [
        "Extensions",
        "Cookies",
        "Passwords",
        "Sync",
        "Restore",
        "Recover",
      ],
      onClick: () =>
        setMainState({
          trigger: {
            name: WhistTrigger.showImportWindow,
            payload: undefined,
          },
        }),
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
}

export { createOptions }
