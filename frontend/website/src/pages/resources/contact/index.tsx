import React from "react"
import classNames from "classnames"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Emails from "@app/pages/resources/contact/emails"
import Careers from "@app/pages/resources/contact/careers"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden"

const Contact = () => {
  return (
    <>
      <div className={classNames(padded, "bg-gray-900")}>
        <Header />
        <Emails />
      </div>
      <div className={classNames(padded, "bg-gray-900")}>
        <Careers />
      </div>
      <Footer />
    </>
  )
}

export default Contact
