import React from "react"
import classNames from "classnames"
import { useLocation } from "react-router"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/download/hero"

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
      <div className={classNames(padded, "bg-gray-900 h-screen")} id="top">
        <Header />
        <Hero
          allowDownloads={
            (location.search.split("show=")?.[1] ?? "") === "download"
          }
        />
      </div>
      <Footer />
    </>
  )
}

export default Download
