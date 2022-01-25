import React from "react"

import { WhistButton, WhistButtonState } from "@app/components/button"
import classNames from "classnames"

const Signout = (props: { onClick: () => void }) => {
  /*
        Description:
            Error pop-up

        Arguments:
            onClick ((clearConfig: boolean) => void): Function to execute when signout button is pressed
    */

  const handleSignout = () => {
    props.onClick()
  }

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-gray-900",
        "justify-center font-body text-center px-12"
      )}
    >
      <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
      <div className="font-semibold text-2xl text-gray-300">
        Are you sure you want to sign out?
      </div>
      <div className="mt-2 mb-4 text-gray-500">
        Signing out will close Whist and you will need to sign back in next
        time. For security, all your browsing data will be cleared.
      </div>
      <div className="w-full">
        <div>
          <WhistButton
            contents="Sign Out"
            className="mt-8 px-12 mx-aut0 py-3 bg-mint text-black"
            state={WhistButtonState.DEFAULT}
            onClick={handleSignout}
          />
        </div>
      </div>
    </div>
  )
}

export default Signout
