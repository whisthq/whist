import React from "react"

import { initGradient } from "@app/renderer/utils/gradient"

const Welcome = (props: { onSubmit: () => void }) => {
  initGradient()

  const onKeyDown = (evt: any) => {
    if (evt.key === "Enter") props.onSubmit()
  }

  return (
    <div
      onKeyDown={onKeyDown}
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none bg-gradient-to-b from-blue-light to-blue"
    >
      <canvas
        id="gradient-canvas"
        className="opacity-0 animate-fade-in-slow"
        style={{ animationDelay: "6000ms" }}
      ></canvas>
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="absolute m-auto text-center mt-24 z-50 w-full h-full mix-blend-overlay">
        <div
          className="text-gray-100 text-6xl mt-20 animate-fade-in opacity-0 font-bold max-w-lg mx-auto animate-translate-up"
          style={{ animationDelay: "1000ms" }}
        >
          Experience a browser without limits
        </div>
        <button
          className="mt-32 text-gray-100 outline-none animate-fade-in opacity-0 bg-white bg-opacity-20 rounded px-8 py-4 tracking-widest"
          onClick={props.onSubmit}
          style={{ animationDelay: "4000ms" }}
          autoFocus={true}
        >
          Login to Continue
        </button>
      </div>
    </div>
  )
}

export default Welcome
