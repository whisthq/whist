import React, { useState } from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"
import classNames from "classnames"

const Signout = (props: { onClick: (clearConfig: boolean) => void }) => {
  /*
        Description:
            Error pop-up

        Arguments:
            onClick ((clearConfig: boolean) => void): Function to execute when signout button is pressed
    */

  const [clearConfig, setClearConfig] = useState(false)

  const handleCheck = () => {
    setClearConfig(!clearConfig)
  }

  const handleSignout = () => {
    props.onClick(clearConfig)
  }

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center",
        "justify-center font-body text-center px-12"
      )}
    >
      <div className="font-semibold text-2xl">
        Are you sure you want to sign out?
      </div>
      <div className="mt-2 mb-4">
        Signing out will terminate your browser and you&lsquo;ll need to sign
        back in nex time.
      </div>
      <div className="w-full">
        <div>
          <FractalButton
            contents="Sign Out"
            className="mt-8 px-12 mx-aut0 py-3"
            state={FractalButtonState.DEFAULT}
            onClick={handleSignout}
          />
        </div>
      </div>
      <div>
        <input type="checkbox" checked={clearConfig} onChange={handleCheck} />
        <div>
          Also permanently wipe all my browsing data (saved passwords, history,
          cookies, etc.).
        </div>
      </div>
    </div>
  )
}

export default Signout
