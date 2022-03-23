import React from "react"

import Logo from "@app/components/icons/logo.svg"
import Landing from "@app/components/templates/landing"

const Introduction = (props: { onSubmit: () => void }) => {
  return (
    <Landing
      logo={
        <img
          src={Logo}
          className="w-24 h-24 m-auto animate-fade-in-up opacity-0"
        />
      }
      title={
        <>
          Welcome to{" "}
          <span className="text-transparent bg-clip-text bg-gradient-to-br from-blue-light to-blue">
            Whist
          </span>
        </>
      }
      subtitle="A quick introduction"
      onSubmit={props.onSubmit}
    />
  )
}

export default Introduction
