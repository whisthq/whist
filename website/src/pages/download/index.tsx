import React from "react"
import classNames from "classnames"
import { useLocation } from "react-router"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/download/hero"
import Requirements from "@app/pages/download/requirements"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden dark"

const Download = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  const location = useLocation()

  return (
    <>
      <div className={classNames(padded, "bg-blue-darkest")} id="top">
        <Header />
        <Hero
          allowDownloads={
            (location.search.split("show=")?.[1] ?? "") === "download"
          }
        />
      </div>
      <div className={classNames(padded, "bg-blue-darker")}>
        <Requirements />
      </div>
      <Footer />
    </>
  )
}

export default Download
