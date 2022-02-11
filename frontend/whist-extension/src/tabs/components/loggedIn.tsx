import React from "react"

const Title = () => (
  <div className="text-gray-300 text-xl animate-fade-in-up opacity-0">
    You are logged in! TODO: Implement what comes next
  </div>
)

export default () => (
  <div className="h-screen w-screen bg-gray-800">
    <div className="flex flex-col h-screen justify-center items-center">
      <Title />
    </div>
  </div>
)
