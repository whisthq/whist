import React, { useEffect } from "react"

const Chatbot = () => {
  useEffect(() => {
    const script = document.createElement("script")
    script.src = `//js.hs-scripts.com/20509395.js`
    script.async = true

    document.body.appendChild(script)
  }, [])

  return (
    <div className="h-screen w-screen bg-gray-800" id="hubspot">
      <div className="w-full h-8 draggable"></div>
    </div>
  )
}

export default Chatbot
