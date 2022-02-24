import React, { useState, useEffect } from "react"

import Steps from "@app/components/steps"
import Introduction from "@app/renderer/pages/onboarding/introduction"
import {
  WhatMakesWhistDifferent,
  WhatIsWhist,
  WhoIsWhistFor,
  FasterLikeMagic,
  LetsShowYouAround,
  Privacy,
  FeaturesUnderDevelopment,
  LiveChatSupport,
  FollowUsOnTwitter,
  TurnOffVPN,
  NetworkTest,
  Pricing,
} from "@app/renderer/pages/onboarding/pages"

const Shuffle = (props: { pages: JSX.Element[]; onSubmit: () => void }) => {
  const maxPageIndex = props.pages.length - 1
  const [pageToShow, setPageToShow] = useState(0)

  useEffect(() => {
    if (pageToShow > maxPageIndex) props.onSubmit()
  }, [pageToShow])

  return (
    <div
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none bg-gray-900"
    >
      {props.pages[pageToShow]}
      <div className="absolute bottom-4 w-full">
        <div className="m-auto text-center">
          <button
            className="text-gray-300 outline-none bg-blue rounded px-12 py-2 font-semibold text-gray-100 w-36 m-auto mb-8 hover:bg-indigo-600"
            onClick={() => setPageToShow(pageToShow + 1)}
          >
            Next
          </button>
          <div className="relative">
            <Steps
              total={maxPageIndex + 1}
              current={pageToShow}
              onClick={(idx: number) => setPageToShow(idx)}
            />
          </div>
        </div>
      </div>
    </div>
  )
}

export default (props: { onSubmit: () => void }) => {
  const [showIntro, setShowIntro] = useState(true)

  if (showIntro)
    return (
      <Introduction
        onSubmit={() => {
          setShowIntro(false)
        }}
      />
    )

  return (
    <Shuffle
      pages={[
        <WhatMakesWhistDifferent />,
        <WhatIsWhist />,
        <WhoIsWhistFor />,
        <FasterLikeMagic />,
        <LetsShowYouAround />,
        <Privacy />,
        <FeaturesUnderDevelopment />,
        <TurnOffVPN />,
        <LiveChatSupport />,
        <Pricing />,
        <FollowUsOnTwitter />,
        <NetworkTest />,
      ]}
      onSubmit={props.onSubmit}
    />
  )
}
