import React from "react"

import Notion from "@app/assets/metadata/notion.json"

const Investors = () => {
  return (
    <div className="mx-auto py-12 max-w-7xl lg:py-24">
      <div className="grid grid-cols-1 gap-12 lg:grid-cols-3 lg:gap-8">
        <div className="space-y-5 sm:space-y-4 pr-6">
          <div className="text-3xl text-gray-300 sm:text-5xl">
            Meet our investors
          </div>
          <p className="text-xl text-gray-500">
            Whist is backed by renowned institutional investors and angels.
          </p>
        </div>
        <div className="lg:col-span-2">
          <ul className="space-y-12 sm:grid sm:grid-cols-2 sm:gap-12 sm:space-y-0 lg:gap-x-8">
            {Notion?.investors?.map((investor) => (
              <li key={investor.name}>
                <a href={investor.website} target="_blank" rel="noreferrer">
                  <div className="flex items-center space-x-4 lg:space-x-6">
                    <img
                      className="w-16 h-16 rounded-full border-2 filter grayscale mr-4"
                      src={investor.imageUrl}
                      alt=""
                    />
                    <div className="font-medium leading-6 space-y-1 text-gray-400">
                      <div className="text-xl">{investor.name}</div>
                      <p className="text-gray-500 text-sm">
                        {investor.description}
                      </p>
                    </div>
                  </div>
                </a>
              </li>
            ))}
          </ul>
        </div>
      </div>
    </div>
  )
}

export default Investors
