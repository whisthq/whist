import React, { useState } from "react"

import Steps from "@app/components/steps"
import Introduction from "@app/renderer/pages/onboarding/introduction"
import {
  WhatMakesWhistDifferent,
  WhatIsWhist,
  WhoIsWhistFor,
} from "@app/renderer/pages/onboarding/pages"

const Shuffle = (props: { pages: JSX.Element[] }) => {
  const maxPageIndex = props.pages.length - 1
  const [pageToShow, setPageToShow] = useState(0)

  const nextPage = () => {
    setPageToShow(Math.min(maxPageIndex, pageToShow + 1))
  }

  return (
    <div
      onKeyDown={nextPage}
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none"
      style={{ background: "#202124" }}
    >
      {props.pages[pageToShow]}
      <div className="absolute bottom-4 w-full">
        <div className="m-auto text-center">
          <button
            className="text-gray-300 outline-none bg-blue rounded px-12 py-2 font-semibold text-gray-100 w-36 m-auto mb-8"
            onClick={nextPage}
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
  const [count, setCount] = useState(0)
  if (count === 0)
    return (
      <Introduction
        onSubmit={() => {
          setCount(1)
        }}
      />
    )
  return (
    <Shuffle
      pages={[<WhatMakesWhistDifferent />, <WhatIsWhist />, <WhoIsWhistFor />]}
    />
  )
}
