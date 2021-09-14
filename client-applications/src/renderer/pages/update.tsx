import React from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"
import classNames from "classnames"

const Update = (props: { onClick: () => void }) => {
  /*
        Description:
            Error pop-up

        Arguments:
            onClick ((clearConfig: boolean) => void): Function to execute when signout button is pressed
    */

  const handleUpdate = () => {
    props.onClick()
  }

  return (
    <div
      className={classNames(
        "flex flex-col h-screen items-center bg-black bg-opacity-80 bg-opacity-90",
        "justify-center font-body text-center px-12"
      )}
    >
      <div className="font-semibold text-2xl text-gray-100">
        A new update is available
      </div>
      <div className="mt-2 mb-4 text-gray-300 max-w-lg">
        If you update now, Fractal will restart and restore your browsing
        session. If you close this window Fractal will update later.
      </div>
      <div className="w-full text-center">
        <div>
          <FractalButton
            contents="Update Now"
            className="mt-6 px-12 py-3 bg-mint text-black cursor-pointer"
            state={FractalButtonState.DEFAULT}
            onClick={handleUpdate}
          />
        </div>
      </div>
    </div>
  )
}

export default Update
