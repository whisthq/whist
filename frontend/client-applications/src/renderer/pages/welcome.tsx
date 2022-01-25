import React from "react"
import Logo from "@app/components/icons/logo.svg"

const Welcome = (props: { onSubmit: () => void }) => {
  const onKeyDown = (evt: any) => {
    if (evt.key === "Enter") props.onSubmit()
  }

  return (
    <div
      onKeyDown={onKeyDown}
      tabIndex={0}
      className="flex flex-col h-screen w-full font-body outline-none bg-gray-900"
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="m-auto text-center mt-36">
        <img
          src={Logo}
          className="w-24 h-24 m-auto animate-fade-in-up opacity-0"
        />
        <div
          className="text-gray-100 text-4xl mt-8 animate-fade-in-up opacity-0"
          style={{ animationDelay: "400ms" }}
        >
          Welcome to <span className="font-bold">Whist</span>
        </div>
        <div
          className="text-gray-400 mt-2 animate-fade-in-up opacity-0"
          style={{ animationDelay: "800ms" }}
        >
          A next-generation cloud-streamed browser
        </div>
        <div
          className="h-px w-32 bg-gray-700 rounded m-auto mt-8 animate-fade-in-up opacity-0"
          style={{ animationDelay: "1200ms" }}
        ></div>
        <button
          className="mt-12 text-gray-100 outline-none animate-fade-in-up opacity-0"
          onClick={props.onSubmit}
          style={{ animationDelay: "2200ms" }}
        >
          <div className="animate-pulse">
            Press{" "}
            <kbd className="bg-gray-700 rounded px-2 py-1 text-xs mx-1">
              Enter
            </kbd>{" "}
            to create an account
          </div>
        </button>
      </div>
    </div>
  )
}

export default Welcome
