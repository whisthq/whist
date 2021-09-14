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
        "flex flex-col h-screen items-center bg-black",
        "justify-center font-body text-center px-12"
      )}
    >
      <div className="font-semibold text-2xl text-gray-300">
        Are you sure you want to sign out?
      </div>
      <div className="mt-2 mb-4 text-gray-500">
        Signing out will close Fractal and you will need to sign back in next
        time.
      </div>
      <div className="w-full">
        <div>
          <FractalButton
            contents="Sign Out"
            className="mt-8 px-12 mx-aut0 py-3 bg-mint text-black"
            state={FractalButtonState.DEFAULT}
            onClick={handleSignout}
          />
        </div>
      </div>
      <div className="flex mt-6 text-gray-500">
        <input
          type="checkbox"
          checked={clearConfig}
          onChange={handleCheck}
          className="mt-1 bg-black bg-opacity-80"
        />
        <div className="ml-3">
          Also permanently erase all my browsing data (history, cookies, etc.)
        </div>
      </div>
    </div>
  )
}

export default Signout
