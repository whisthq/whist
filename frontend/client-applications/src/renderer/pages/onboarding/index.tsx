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
import { WhistButton, WhistButtonState } from "@app/components/button"

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
          <WhistButton
            onClick={() => setPageToShow(pageToShow + 1)}
            state={WhistButtonState.DEFAULT}
            contents="Next"
            className="mb-8 px-16"
          />
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

const Onboarding = (props: { onSubmit: () => void }) => {
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
        <WhatMakesWhistDifferent key={0} />,
        <WhatIsWhist key={1} />,
        <WhoIsWhistFor key={2} />,
        <FasterLikeMagic key={3} />,
        <LetsShowYouAround key={4} />,
        <Privacy key={5} />,
        <FeaturesUnderDevelopment key={6} />,
        <TurnOffVPN key={7} />,
        <LiveChatSupport key={8} />,
        <Pricing key={9} />,
        <FollowUsOnTwitter key={10} />,
        <NetworkTest key={11} />,
      ]}
      onSubmit={props.onSubmit}
    />
  )
}

export default Onboarding
