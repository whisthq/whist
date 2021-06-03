import React from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"
import classNames from "classnames"

const Signout = (props: { onClick: () => void }) => {
  /*
        Description:
            Error pop-up

        Arguments:
            onClick (() => void): Function to execute when signout button is pressed
    */

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
        To protect your privacy, signing out will <i>permanently</i> delete your
        Fractal browsing session data, including any saved passwords, cookies,
        history, etc.
      </div>
      <div className="w-full">
        <div>
          <FractalButton
            contents="Sign Out"
            className="mt-8 px-12 mx-aut0 py-3"
            state={FractalButtonState.DEFAULT}
            onClick={props.onClick}
          />
        </div>
      </div>
    </div>
  )
}

export default Signout
