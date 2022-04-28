import React from "react"
import ReactDOM from "react-dom"

import Auth from "@app/tabs/components/auth"
import LoggedIn from "@app/tabs/components/loggedIn"
import NotFound from "@app/tabs/components/notFound"

import { authTab, loggedInTab } from "@app/constants/tabs"

const show = window.location.search.split("show=")[1]

const Tab = () => {
  if (show === authTab) return <Auth />
  if (show === loggedInTab) return <LoggedIn />
  return <NotFound />
}

ReactDOM.render(<Tab />, document.querySelector("#root"))
