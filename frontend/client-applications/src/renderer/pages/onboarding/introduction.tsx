import React from "react"

import Logo from "@app/components/icons/logo.svg"
import { WhistButton, WhistButtonState } from "@app/components/button"

const Introduction = (props: { onSubmit: () => void }) => {
  return (
    <div
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none bg-gray-900"
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="m-auto text-center mt-18">
        <img
          src={Logo}
          className="w-24 h-24 m-auto animate-fade-in-up opacity-0"
        />
        <div
          className="text-gray-100 text-4xl mt-8 animate-fade-in-up opacity-0"
          style={{ animationDelay: "400ms" }}
        >
          Welcome to{" "}
          <span className="font-bold text-transparent bg-clip-text bg-gradient-to-br from-blue-light to-blue">
            Whist
          </span>
        </div>
        <div
          className="text-gray-400 mt-2 animate-fade-in-up opacity-0"
          style={{ animationDelay: "800ms" }}
        >
          First, an introduction
        </div>
        <div
          className="h-px w-32 bg-gray-700 rounded m-auto mt-8 animate-fade-in-up opacity-0"
          style={{ animationDelay: "1200ms" }}
        ></div>
        <div
          className="animate-fade-in-up opacity-0 mt-20 relative top-1"
          style={{ animationDelay: "2000ms" }}
        >
          <WhistButton
            onClick={props.onSubmit}
            state={WhistButtonState.DEFAULT}
            contents="Next"
            className="px-16"
          />
        </div>
      </div>
    </div>
  )
}

export default Introduction
