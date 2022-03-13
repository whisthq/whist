import React from "react"
import ReactDOM from "react-dom"

import Welcome from "./welcome"

const Root = () => {
  return <Welcome />
}

ReactDOM.render(<Root />, document.querySelector("#root"))
