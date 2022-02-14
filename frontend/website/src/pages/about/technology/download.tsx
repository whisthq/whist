import React from "react"

import { WhistButton, WhistButtonState } from "@app/shared/components/button"

const Download = () => {
  return (
    <div className="max-w-7xl mx-auto mt-12 py-12 px-4 sm:py-16 lg:py-20">
      <div className="max-w-4xl mx-auto text-center">
        <div className="text-3xl text-gray-300 md:text-5xl">
          Chrome, just faster
        </div>
        <p className="mt-3 text-xl text-gray-500 sm:mt-4">
          Whist runs Google Chrome on datacenter-grade hardware
        </p>
      </div>
      <div className="text-center mt-12">
        <a href="/download#top">
          <WhistButton
            contents="Get Started"
            state={WhistButtonState.DEFAULT}
          />
        </a>
      </div>
    </div>
  )
}

export default Download
