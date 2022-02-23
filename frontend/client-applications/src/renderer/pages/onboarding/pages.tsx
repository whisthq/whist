import React from "react"

import { TypeWriter } from "@app/components/typewriter"
import ChromeHeader from "@app/components/assets/chromeHeader.svg"
import Computer from "@app/components/icons/computer"
import Duplicate from "@app/components/icons/duplicate"
import Globe from "@app/components/icons/globe"

const Template = (props: { contents: JSX.Element; word: string }) => {
  return (
    <div
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none"
      style={{ background: "#202124" }}
    >
      <img src={ChromeHeader} className="w-screen nondraggable" />
      {props.word !== undefined && (
        <div className="absolute left-28" style={{ top: 40.5 }}>
          <TypeWriter
            words={[props.word]}
            classNameCursor="bg-gray-300"
            classNameText="text-gray-300 text-xs font-semibold"
            startDelay={50}
          />
        </div>
      )}
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div
        className="animate-fade-in-up opacity-0"
        style={{ animationDelay: `${props.word.length * 95}ms` }}
      >
        {props.contents}
      </div>
    </div>
  )
}

const WhatMakesWhistDifferent = () => {
  const contents = (
    <div className="m-auto text-center mt-18">
      <div className="mt-32 text-gray-300 text-3xl font-bold leading-10">
        What makes Whist different <br />
        from other browsers?
      </div>
    </div>
  )

  return <Template contents={contents} word="You may be wondering . . ." />
}

const WhatIsWhist = () => {
  const contents = (
    <div className="m-auto text-center mt-18">
      <div className="mt-32 text-gray-300 text-3xl font-bold leading-10 max-w-md m-auto">
        Whist uses <span className="text-mint">almost zero memory</span>&nbsp;
        and loads websites at lightning speed
      </div>
    </div>
  )

  return <Template contents={contents} word="Powered by the cloud," />
}

const WhoIsWhistFor = () => {
  const features = [
    {
      name: "Slow or old computers",
      icon: Computer,
    },
    {
      name: "Hundreds of tabs",
      icon: Duplicate,
    },
    {
      name: "Heavy, slow web apps",
      icon: Globe,
    },
  ]

  const icons = (
    <dl className="grid grid-cols-3 space-x-4">
      {features.map((feature) => (
        <div
          key={feature.name}
          className="text-left bg-gray-800 p-4 rounded-md"
        >
          <dt>
            <div className="flex items-center justify-center h-10 w-10 rounded-md bg-blue text-gray-300">
              <feature.icon aria-hidden="true" />
            </div>
            <p className="mt-5 text-sm text-gray-300">{feature.name}</p>
          </dt>
        </div>
      ))}
    </dl>
  )

  const contents = (
    <div className="m-auto text-center mt-20">
      <div className="mt-16 text-gray-300 text-xl font-bold leading-10 max-w-lg m-auto text-center">
        For those who struggle with their browser&apos;s speed
      </div>
      <div className="max-w-lg m-auto px-10 py-6">{icons}</div>
    </div>
  )

  return <Template contents={contents} word="Who is Whist for?" />
}

export { WhatMakesWhistDifferent, WhatIsWhist, WhoIsWhistFor }
