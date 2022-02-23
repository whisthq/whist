import React from "react"

import { TypeWriter } from "@app/components/typewriter"
import ChromeHeader from "@app/components/assets/chromeHeader.svg"
import Computer from "@app/components/icons/computer"
import Duplicate from "@app/components/icons/duplicate"
import Globe from "@app/components/icons/globe"
import Star from "@app/components/icons/star"
import Database from "@app/components/icons/database"
import Play from "@app/components/icons/play"
import Lock from "@app/components/icons/lock"
import Camera from "@app/components/icons/camera"
import Server from "@app/components/icons/server"

const Template = (props: { contents: JSX.Element; word: string }) => {
  return (
    <div
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none bg-gray-900"
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

  return (
    <Template
      contents={contents}
      word="Hey there! You may be wondering . . ."
    />
  )
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

  return <Template contents={contents} word="Powered by cloud streaming," />
}

const WhoIsWhistFor = () => {
  const features = [
    {
      name: "A slow or old computer",
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
        For those who struggle with:
      </div>
      <div className="max-w-lg m-auto px-10 py-6">{icons}</div>
    </div>
  )

  return <Template contents={contents} word="Who is Whist for?" />
}

const FasterLikeMagic = () => {
  const contents = (
    <div className="m-auto text-center mt-18">
      <div className="mt-32 text-gray-300 text-3xl font-bold leading-10 max-w-md m-auto">
        Using Whist will make your computer{" "}
        <span className="text-mint">faster</span> like
        <div className="inline-block pl-2 pr-1 text-yellow-200">
          <Star />
        </div>
        magic
        <div className="inline-block pr-2 pl-1 text-yellow-200">
          <Star />
        </div>
      </div>
    </div>
  )

  return <Template contents={contents} word="If these problems describe you," />
}

const LetsShowYouAround = () => {
  const contents = (
    <div className="m-auto text-center mt-18">
      <div className="mt-32 text-gray-300 text-3xl font-bold leading-10 max-w-sm m-auto">
        Here are some important things to know about your <br />
        <span className="text-mint">shiny new browser</span>
      </div>
    </div>
  )

  return (
    <Template
      contents={contents}
      word="Anyway, enough with the sales pitch :)"
    />
  )
}

const Privacy = () => {
  const features = [
    {
      name: "Your browsing data is encrypted by a master key cached on your computer, not our servers",
      icon: Database,
    },
    {
      name: "Your video and audio are encrypted, never stored, and cannot be seen by anyone but you.",
      icon: Play,
    },
    {
      name: "Our encryption ensures that no one, not even our engineers, have access to your data.",
      icon: Lock,
    },
  ]

  const icons = (
    <dl className="grid grid-cols-3 space-x-4">
      {features.map((feature) => (
        <div
          key={feature.name}
          className="text-left bg-gray-800 p-5 rounded-md"
        >
          <dt>
            <div className="flex items-center justify-center h-8 w-8 rounded-md bg-blue text-gray-300 p-1">
              <feature.icon aria-hidden="true" />
            </div>
            <p className="mt-6 text-sm text-gray-400">{feature.name}</p>
          </dt>
        </div>
      ))}
    </dl>
  )

  const contents = (
    <div className="m-auto text-center mt-10">
      <div className="text-gray-300 text-xl font-bold leading-10 max-w-xl m-auto text-center">
        Whist protects your online privacy as a human right
      </div>
      <div className="max-w-xl m-auto px-10 py-6">{icons}</div>
    </div>
  )

  return (
    <Template
      contents={contents}
      word="How does Whist handle privacy and security?"
    />
  )
}

const FeaturesUnderDevelopment = () => {
  const features = [
    {
      name: "Support for multiple browser windows",
      icon: Duplicate,
    },
    {
      name: "Support for Google Meets camera + microphone",
      icon: Camera,
    },
    {
      name: "Support for running localhost",
      icon: Server,
    },
  ]

  const icons = (
    <dl className="grid grid-cols-3 space-x-4">
      {features.map((feature) => (
        <div
          key={feature.name}
          className="text-left bg-gray-800 p-5 rounded-md"
        >
          <dt>
            <div className="flex items-center justify-center h-8 w-8 rounded-md bg-blue text-gray-300 p-1">
              <feature.icon aria-hidden="true" />
            </div>
            <p className="mt-6 text-sm text-gray-400">{feature.name}</p>
          </dt>
        </div>
      ))}
    </dl>
  )

  const contents = (
    <div className="m-auto text-center mt-10">
      <div className="text-gray-300 text-xl font-bold leading-8 max-w-lg m-auto text-center">
        These features are being developed and will become available in the
        coming months
      </div>
      <div className="max-w-xl m-auto px-10 py-6">{icons}</div>
    </div>
  )

  return (
    <Template
      contents={contents}
      word="Are there any features under development?"
    />
  )
}

const LiveChatSupport = () => {
  const contents = (
    <div className="m-auto text-center mt-18">
      <div className="mt-32 text-gray-300 text-2xl font-bold leading-10 max-w-md m-auto">
        After this onboarding, the{" "}
        <kbd className="bg-gray-700 px-2 py-1 rounded">Cmd</kbd>&nbsp;
        <kbd className="bg-gray-700 px-2 py-1 rounded">J</kbd> shortcut summons
        the Whist menu, which features live chat support.
      </div>
    </div>
  )

  return (
    <Template contents={contents} word="What should I do if I need help?" />
  )
}

const FollowUsOnTwitter = () => {
  const contents = (
    <div className="m-auto text-center mt-18">
      <div className="mt-20 text-gray-300 text-2xl font-bold leading-10 max-w-md m-auto">
        Follow us on Twitter&nbsp;
        <div onClick={() => {}} className="inline-block">
          <kbd className="bg-gray-700 px-2 py-1 rounded">@whisthq</kbd>
        </div>
        . <br />
        Have a new feature request?{" "}
        <span className="text-blue-400">Tweet us!</span>
        <br /> Want to write us a poem?{" "}
        <span className="text-blue-400">Tweet us!</span>
        <br /> Want us to write you a poem?{" "}
        <span className="text-blue-400">Tweet us!</span>
      </div>
      <div className="mt-4 text-md text-gray-500">
        (Seriously, we&apos;ll write you one)
      </div>
    </div>
  )

  return (
    <Template
      contents={contents}
      word="How can I keep in touch with the Whist team?"
    />
  )
}

export {
  WhatMakesWhistDifferent,
  WhatIsWhist,
  WhoIsWhistFor,
  FasterLikeMagic,
  LetsShowYouAround,
  Privacy,
  FeaturesUnderDevelopment,
  LiveChatSupport,
  FollowUsOnTwitter,
}
