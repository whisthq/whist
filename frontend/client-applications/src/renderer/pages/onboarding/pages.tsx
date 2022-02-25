import React, { useEffect } from "react"

import { TypeWriter } from "@app/components/typewriter"
import { Network } from "@app/components/network"

import Computer from "@app/components/icons/computer"
import Duplicate from "@app/components/icons/duplicate"
import Globe from "@app/components/icons/globe"
import Star from "@app/components/icons/star"
import Database from "@app/components/icons/database"
import Play from "@app/components/icons/play"
import Lock from "@app/components/icons/lock"
import Camera from "@app/components/icons/camera"
import Server from "@app/components/icons/server"

import { useMainState } from "@app/renderer/utils/ipc"
import { WhistTrigger } from "@app/constants/triggers"

import ChromeHeader from "@app/components/assets/chromeHeader.svg"

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
            classNameText="text-gray-300 text-xs font-bold"
            startDelay={0}
          />
        </div>
      )}
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div
        className="animate-fade-in-up opacity-0"
        style={{ animationDelay: `${props.word.length * 40}ms` }}
      >
        {props.contents}
      </div>
    </div>
  )
}

const WhatMakesWhistDifferent = () => {
  const [, setMainState] = useMainState()

  useEffect(() => {
    setMainState({
      trigger: {
        name: WhistTrigger.startNetworkAnalysis,
        payload: undefined,
      },
    })
  }, [])

  const contents = (
    <div className="m-auto text-center">
      <div className="mt-44 text-gray-300 text-3xl font-bold leading-10">
        What makes <span className="text-blue-light">Whist</span> different{" "}
        <br />
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
      <div className="mt-44 text-gray-300 text-3xl font-bold leading-10 max-w-md m-auto">
        Whist uses <span className="text-blue-light">almost zero memory</span>{" "}
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
      name: "Conserving battery life",
      icon: Duplicate,
    },
    {
      name: "A sea of tabs or slow websites",
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
            <div className="flex items-center justify-center h-10 w-10 rounded-md bg-blue-light text-gray-800">
              <feature.icon aria-hidden="true" />
            </div>
            <p className="mt-5 text-sm text-gray-300">{feature.name}</p>
          </dt>
        </div>
      ))}
    </dl>
  )

  const contents = (
    <div className="m-auto text-center mt-16">
      <div className="mt-28 text-gray-300 text-2xl font-bold leading-10 max-w-lg m-auto text-center">
        Whist is for you if you struggle with
      </div>
      <div className="max-w-lg m-auto px-10 py-6">{icons}</div>
    </div>
  )

  return <Template contents={contents} word="Who is Whist built for?" />
}

const FasterLikeMagic = () => {
  const contents = (
    <div className="m-auto text-center mt-18">
      <div className="mt-44 text-gray-300 text-3xl font-bold leading-10 max-w-md m-auto">
        Using Whist will make your computer{" "}
        <span className="text-blue-light">faster</span> and{" "}
        <span className="text-blue-light">quieter</span> like
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
      <div className="mt-44 text-gray-300 text-3xl font-bold leading-10 max-w-sm m-auto">
        Here are some important things to know about your{" "}
        <span className="text-blue-light">shiny new browser</span>
      </div>
    </div>
  )

  return <Template contents={contents} word="Now let's get you set up!" />
}

const Privacy = () => {
  const features = [
    {
      name: "Your browsing data is encrypted by a master key cached on your computer, not our servers",
      icon: Database,
    },
    {
      name: "Your video stream is encrypted, never stored, and cannot be seen by anyone.",
      icon: Play,
    },
    {
      name: "Encryption ensures that no one, not even our engineers, can access your data.",
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
            <div className="flex items-center justify-center h-8 w-8 rounded-md bg-blue-light text-gray-800 p-1">
              <feature.icon aria-hidden="true" />
            </div>
            <p className="mt-6 text-sm text-gray-400">{feature.name}</p>
          </dt>
        </div>
      ))}
    </dl>
  )

  const contents = (
    <div className="m-auto text-center mt-24">
      <div className="text-gray-300 text-2xl font-bold leading-10 max-w-lg m-auto text-center">
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
      name: "Support for Google Meets camera + mic",
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
            <div className="flex items-center justify-center h-8 w-8 rounded-md bg-blue-light text-gray-800 p-1">
              <feature.icon aria-hidden="true" />
            </div>
            <p className="mt-6 text-sm text-gray-400">{feature.name}</p>
          </dt>
        </div>
      ))}
    </dl>
  )

  const contents = (
    <div className="m-auto text-center mt-28">
      <div className="text-gray-300 text-2xl font-bold leading-8 max-w-lg m-auto text-center">
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

const TurnOffVPN = () => {
  const contents = (
    <div className="m-auto text-center max-w-md mt-44">
      <div className="text-gray-300 text-2xl font-bold leading-10 m-auto">
        If you use a VPN, please whitelist{" "}
        <span className="text-blue-light">Whist</span>
      </div>
      <div className="mt-2 text-md text-gray-500">
        VPNs significantly degrade Whist&apos;s performance. From a privacy
        perspective, Whist already acts as a VPN by sending all web requests
        from a secure server instead of your computer.
      </div>
    </div>
  )

  return (
    <Template contents={contents} word="Can I run my VPN on top of Whist?" />
  )
}

const LiveChatSupport = () => {
  const contents = (
    <div className="m-auto text-center mt-18 max-w-md m-auto">
      <div className="mt-44 text-gray-300 text-2xl font-bold leading-10">
        After this onboarding,{" "}
        <kbd className="bg-gray-700 px-2 py-1 rounded">Cmd</kbd>&nbsp;
        <kbd className="bg-gray-700 px-2 py-1 rounded">J</kbd> summons the Whist
        menu.
      </div>
      <div className="mt-2 text-md text-gray-500">
        Here you&apos;ll find live chat support for bug reports; account and
        settings; helpful tools like tab and bookmark importing, and more!
      </div>
    </div>
  )

  return (
    <Template contents={contents} word="What should I do if I need help?" />
  )
}

const FollowUsOnTwitter = () => {
  const [, setMainState] = useMainState()
  const openTwitterURL = () => {
    setMainState({
      trigger: {
        name: WhistTrigger.openExternalURL,
        payload: { url: "https://twitter.com/whisthq" },
      },
    })
  }
  const contents = (
    <div className="m-auto text-center mt-44">
      <div className="mt-20 text-gray-300 text-xl font-bold leading-10 max-w-md m-auto">
        Please follow us on Twitter&nbsp;
        <div onClick={openTwitterURL} className="inline-block">
          <kbd className="px-2 py-1 rounded cursor-pointer bg-blue hover:bg-blue-dark duration-75">
            @whisthq
          </kbd>
        </div>
        . <br />
        Have a new feature request?{" "}
        <span
          className="text-blue-light cursor-pointer"
          onClick={openTwitterURL}
        >
          Tweet us!
        </span>
        <br /> Want to write us a poem?{" "}
        <span
          className="text-blue-light cursor-pointer"
          onClick={openTwitterURL}
        >
          Tweet us!
        </span>
        <br /> Want us to write you a poem?{" "}
        <span
          className="text-blue-light cursor-pointer"
          onClick={openTwitterURL}
        >
          Tweet us!
        </span>
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

const NetworkTest = () => {
  const [mainState, setMainState] = useMainState()

  useEffect(() => {
    setMainState({
      trigger: { name: WhistTrigger.emitIPC, payload: undefined },
    })
  }, [])

  const contents = (
    <div className="mt-28">
      <Network
        networkInfo={mainState.networkInfo}
        onSubmit={() => {}}
        withText={true}
        withButton={false}
      />
    </div>
  )

  return (
    <Template
      contents={contents}
      word="You need good Internet for Whist. Let's test your Internet!"
    />
  )
}

const Pricing = () => {
  const contents = (
    <div className="m-auto text-center mt-44 max-w-md m-auto">
      <div className="text-gray-300 text-2xl font-bold leading-10">
        For the next two weeks,{" "}
        <span className="text-blue-light">Whist is free</span>. Afterward, Whist
        is <span className="text-xs relative bottom-2">$</span>9
        <span className="text-xs">/mo</span>
      </div>
      <div className="mt-4 text-md text-gray-500">
        You can cancel at any time and we will refund you if you forget. After
        this onboarding,{" "}
        <kbd className="bg-gray-700 px-2 py-1 mx-1 rounded text-xs">Cmd</kbd>
        <kbd className="bg-gray-700 px-2 py-1 mx-1 rounded text-xs">J</kbd> also
        gives you access to billing info.
      </div>
    </div>
  )

  return <Template contents={contents} word="Whist is made with <3" />
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
  NetworkTest,
  TurnOffVPN,
  Pricing,
}
