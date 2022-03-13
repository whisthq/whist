import React from "react"
import ReactDOM from "react-dom"

import Welcome from "./welcome"

const Root = () => {
  return (
    <div className="relative font-body">
      <Welcome />
    </div>
  )
}

ReactDOM.render(<Root />, document.querySelector("#root"))
