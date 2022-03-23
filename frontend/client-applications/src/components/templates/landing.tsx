import React from "react"

import { WhistButton, WhistButtonState } from "@app/components/button"

const Landing = (props: {
  logo: JSX.Element
  title: string | JSX.Element
  subtitle: string | JSX.Element
  onSubmit: () => void
}) => {
  return (
    <div
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none bg-gray-900"
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="m-auto text-center relative bottom-16">
        {props.logo}
        <div
          className="text-gray-100 text-4xl mt-8 animate-fade-in-up opacity-0 font-bold"
          style={{ animationDelay: "400ms" }}
        >
          {props.title}
        </div>
        <div
          className="text-gray-400 mt-4 animate-fade-in-up opacity-0"
          style={{ animationDelay: "800ms" }}
        >
          {props.subtitle}
        </div>
        <div
          className="h-px w-32 bg-gray-700 rounded m-auto mt-8 animate-fade-in-up opacity-0"
          style={{ animationDelay: "1200ms" }}
        ></div>
      </div>
      <div
        className="absolute bottom-8 left-0 w-full animate-fade-in-up opacity-0"
        style={{ animationDelay: "2000ms" }}
      >
        <div className="m-auto text-center">
          <WhistButton
            onClick={props.onSubmit}
            state={WhistButtonState.DEFAULT}
            contents="Continue"
            className="mb-9 px-16 w-60"
          />
        </div>
      </div>
    </div>
  )
}

export default Landing
