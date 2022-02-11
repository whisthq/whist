import React from "react"

import { openGoogleAuth } from "@app/tabs/utils/auth"

const LoginTitle = () => (
  <div
    className="text-gray-200 text-8xl font-bold text-transparent bg-clip-text bg-gradient-to-br from-purple-500 to-pink-500 animate-fade-in-up opacity-0"
    style={{ animationDelay: "400ms" }}
  >
    Welcome to Whist
  </div>
)

const LoginSubtitle = () => (
  <div
    className="text-gray-300 text-xl animate-fade-in-up opacity-0"
    style={{ animationDelay: "800ms" }}
  >
    To unlock cloud tabs, please sign in or create an account
  </div>
)

const LoginButton = (props: { login: () => void }) => (
  <button
    onClick={props.login}
    className="text-gray-800 text-xl font-bold outline-none rounded px-16 py-4 bg-pink-500 animate-fade-in-up opacity-0"
    style={{ animationDelay: "1600ms" }}
  >
    Log in
  </button>
)

export default () => {
  return (
    <div className="w-screen h-screen bg-gray-800 font-body">
      <div className="flex flex-col h-screen justify-center items-center">
        <div>
          <LoginTitle />
        </div>
        <div className="mt-4">
          <LoginSubtitle />
        </div>
        <div className="mt-16">
          <LoginButton login={openGoogleAuth} />
        </div>
      </div>
    </div>
  )
}
