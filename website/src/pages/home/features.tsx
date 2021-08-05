import React from "react"
import {
  LightningBoltIcon,
  ChipIcon,
  CloudDownloadIcon,
} from "@heroicons/react/outline"

const supportLinks = [
  {
    name: "Your Google Environment",
    description:
      "Signing into your Google account on Fractal loads all your extensions and settings.",
    icon: CloudDownloadIcon,
  },
  {
    name: "Lightning Speeds",
    description:
      "Fractal’s proprietary streaming technology runs at 60+ frames per second with no noticeable lag.",
    icon: LightningBoltIcon,
  },
  {
    name: "Dedicated GPU",
    description:
      "Fractal runs Chrome on a dedicated NVIDIA graphics card for the fastest page rendering speeds.",
    icon: ChipIcon,
  },
]

export const Features = () => {
  /*
        Features section of Chrome product page

        Arguments:
            none
    */

  return (
    <div className="mt-32 text-center">
      <div className="pt-24">
        <div className="text-3xl md:text-5xl dark:text-gray-300 leading-normal">
          The first{" "}
          <span className="text-blue dark:text-mint">cloud-powered</span> Chrome
        </div>
      </div>
      <p className="mt-3 text-xl text-gray-500 sm:mt-4 mb-12">
        Fractal is the Google Chrome you love, running at breathtaking speeds in
        the cloud.
      </p>
      <div className="grid grid-cols-1 gap-y-6 lg:grid-cols-3 lg:gap-y-0 lg:gap-x-8 m-auto text-left max-w-7xl">
        {supportLinks.map((link) => (
          <div key={link.name} className="flex flex-col rounded bg-gray-900">
            <div className="flex-1 relative pt-16 px-6 pb-8 md:px-8">
              <div className="top-0 p-3 inline-block bg-blue rounded transform -translate-y-1/2">
                <link.icon className="h-5 w-5 text-white" aria-hidden="true" />
              </div>
              <h3 className="text-xl font-medium text-gray-300">{link.name}</h3>
              <p className="mt-4 text-base text-gray-500">{link.description}</p>
            </div>
          </div>
        ))}
      </div>
      <a href="/technology#top">
        <button className="mt-12 text-mint py-2 px-8 rounded-3xl mb-12 hover:text-mint">
          Read more about the technology
        </button>
      </a>
    </div>
  )
}

export default Features
