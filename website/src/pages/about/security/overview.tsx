import React from "react"

import {
  DatabaseIcon,
  BookOpenIcon,
  PlayIcon,
  KeyIcon,
} from "@heroicons/react/outline"

const features = [
  {
    name: "Browsing History",
    description:
      "Your browsing history is locked behind a master key that only you have access to because it is cached to your computer, not our servers.",
    icon: BookOpenIcon,
  },
  {
    name: "Passwords and Cookies",
    description:
      "All your saved passwords and cookies are fully encrypted and locked behind the same master key.",
    icon: KeyIcon,
  },
  {
    name: "Video and Audio",
    description:
      "Your video and audio packets are encrypted along every step of the streaming process and are never tracked, stored, or seen by anyone besides you.",
    icon: PlayIcon,
  },
  {
    name: "Account Data",
    description:
      "Even if we wanted to, we couldn’t sell any of your data to a third-party service. That’s how serious we are about privacy.",
    icon: DatabaseIcon,
  },
]

const Overview = () => {
  return (
    <div className="py-16 mt-36 lg:mt-12 bg-gray-800">
      <div className="max-w-7xl mx-auto px-8">
        <div className="lg:text-center">
          <p className="mt-2 text-2xl leading-8 text-gray-300 lg:text-4xl">
            Overview
          </p>
          <p className="mt-2 max-w-2xl text-lg lg:text-xl text-gray-400 lg:mx-auto">
            Fractal believes that privacy is a fundamental human right and
            ensures that nobody but you can view your browsing information. No
            engineer can view personal information about your browsing session.
            In particular:
          </p>
        </div>

        <div className="mt-16">
          <dl className="space-y-10 md:space-y-0 md:grid md:grid-cols-2 md:gap-x-8 md:gap-y-10">
            {features.map((feature) => (
              <div key={feature.name} className="relative">
                <dt>
                  <div className="absolute flex items-center justify-center h-12 w-12 rounded-md bg-indigo-500 text-white">
                    <feature.icon className="h-6 w-6" aria-hidden="true" />
                  </div>
                  <p className="ml-20 text-lg leading-6 font-medium text-gray-300">
                    {feature.name}
                  </p>
                </dt>
                <dd className="ml-20 text-base text-gray-500">
                  {feature.description}
                </dd>
              </div>
            ))}
          </dl>
        </div>
      </div>
    </div>
  )
}

export default Overview
