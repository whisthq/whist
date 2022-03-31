/*
    The entry point to our React code which runs inside custom pages.
*/

import React from "react"
import ReactDOM from "react-dom"
import { Helmet } from "react-helmet"

import Welcome from "./welcome"

const Root = () => {
  return (
    <div className="relative font-body">
      <Helmet>
        <title>Welcome to Whist</title>
      </Helmet>
      <Welcome />
    </div>
  )
}

ReactDOM.render(<Root />, document.querySelector("#root"))
