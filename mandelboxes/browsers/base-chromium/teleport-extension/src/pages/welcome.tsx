import React from "react"

import Logo from "./assets/whist_logo_128.png"

const Welcome = () => (
  <div className="flex w-screen h-screen bg-gray-900 justify-center items-center relative">
    <div className="text-center relative bottom-16">
      <img
        src={Logo}
        className="w-16 h-16 mx-auto opacity-0 animate-fade-in-up"
      />
      <div
        className="mt-12 text-7xl font-bold text-gray-300 opacity-0 animate-fade-in-up"
        style={{ animationDelay: "600ms" }}
      >
        Welcome to{" "}
        <span className="text-transparent bg-clip-text bg-gradient-to-br from-blue-light to-blue">
          Whist
        </span>
      </div>
      <div
        className="mt-8 text-gray-400 text-lg opacity-0 animate-fade-in-up"
        style={{ animationDelay: "1200ms" }}
      >
        Whist loads pages on the fastest Internet you&apos;ll ever use, period.
      </div>
    </div>
    <a
      href="https://fast.com"
      target="_blank"
      className="outline-none absolute bottom-12 mx-auto opacity-0 animate-fade-in-up"
      style={{ animationDelay: "2000ms" }}
    >
      <button className="px-12 py-4 text-gray-300 text-lg font-bold animate-pulse">
        Show me the speed
      </button>
    </a>
  </div>
)

export default Welcome
