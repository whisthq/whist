import React from "react"

const NotFoundTitle = () => (
  <div
    className="text-gray-200 text-8xl font-bold text-transparent bg-clip-text bg-gradient-to-br from-purple-500 to-pink-500 animate-fade-in-up opacity-0"
    style={{ animationDelay: "400ms" }}
  >
    404 Not Found
  </div>
)

const NotFoundSubtitle = () => (
  <div
    className="text-gray-300 text-xl animate-fade-in-up opacity-0"
    style={{ animationDelay: "800ms" }}
  >
    This is an error on our end and we sincerely apologize for the
    inconvenience.
  </div>
)

export default () => (
  <div className="h-screen w-screen bg-gray-800">
    <div className="flex flex-col h-screen justify-center items-center">
      <div>
        <NotFoundTitle />
      </div>
      <div className="mt-4">
        <NotFoundSubtitle />
      </div>
    </div>
  </div>
)
