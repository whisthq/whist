import React from "react"

import Landing from "@app/components/templates/landing"
import Logo from "@app/components/icons/logo.svg"

const Welcome = (props: { onSubmit: () => void }) => {
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
          Sign in to{" "}
          <span className="text-transparent bg-clip-text bg-gradient-to-br from-blue-light to-blue">
            Whist
          </span>
        </>
      }
      subtitle="A next-generation cloud browser"
      onSubmit={props.onSubmit}
    />
  )
}

export default Welcome
