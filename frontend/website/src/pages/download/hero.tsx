import React, { useState } from "react"
import { FaApple } from "react-icons/fa"

import Popover from "@app/shared/components/popover"
import { WhistButton, WhistButtonState } from "@app/shared/components/button"
import { config } from "@app/shared/constants/config"
import Widget from "@app/pages/download/widget"

const getOperatingSystem = (window: any) => {
  let operatingSystem = "MacOS"
  if (window.navigator.appVersion.indexOf("Win") !== -1) {
    operatingSystem = "Windows"
  }
  if (window.navigator.appVersion.indexOf("Mac") !== -1) {
    operatingSystem = "MacOS"
  }
  if (window.navigator.appVersion.indexOf("Linux") !== -1) {
    operatingSystem = "Linux"
  }

  return operatingSystem
}

const Hero = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  const [open, setOpen] = useState(false)
  const operatingSystem = getOperatingSystem(window)

  return (
    <div className="mb-24 text-center pt-36">
      <div className="px-0 md:px-12">
        <Widget open={open} setOpen={setOpen} />
        <div className="text-6xl md:text-7xl tracking-wide leading-snug text-gray dark:text-gray-300 mb-4 max-w-4xl mx-auto">
          A <span className="text-blue-light">faster, lighter</span> browser
        </div>
        {operatingSystem === "MacOS" ? (
          <div className="text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-3xl m-auto">
            In order to use Whist, you must have{" "}
            <span className="text-blue-light">macOS 10.13+</span> and be{" "}
            <span className="text-blue-light">located in North America</span>.
            If you do not meet these requirements, please{" "}
            <span
              onClick={() => setOpen(true)}
              className="text-blue-light cursor-pointer"
            >
              leave us your email
            </span>{" "}
            and we will notify you when we can support you!
          </div>
        ) : (
          <div className="text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-3xl m-auto">
            Whist is currently Mac-only. Our team is working hard on releasing{" "}
            {operatingSystem} support within the next few months. We will notify
            you as soon as we are ready if you drop your email below.
          </div>
        )}
        {operatingSystem === "MacOS" ? (
          <>
            {" "}
            <div className="md:flex md:space-x-4 space-y-4 md:space-y-0 justify-center mt-12">
              <Popover
                element={
                  <a href={config.client_download_urls.macOS_x64} download>
                    <WhistButton
                      contents={
                        <div className="flex">
                          <FaApple className="relative mr-3 top-0.5" />
                          macOS (Intel)
                        </div>
                      }
                      state={WhistButtonState.DEFAULT}
                    />
                  </a>
                }
                popup={
                  <div>
                    If your Mac was made{" "}
                    <span className="font-semibold text-gray-300">
                      before November 2020
                    </span>
                    , it has an Intel chip. To confirm, click the{" "}
                    <FaApple className="inline-block relative bottom-0.5 text-gray-300" />{" "}
                    icon in the top left corner of your screen, select
                    &lsquo;About This Mac&lsquo;, and see the chip description.
                  </div>
                }
              />
              <Popover
                element={
                  <a href={config.client_download_urls.macOS_arm64} download>
                    <WhistButton
                      contents={
                        <div className="flex">
                          <FaApple className="relative mr-3 top-0.5" />
                          macOS (M1)
                        </div>
                      }
                      state={WhistButtonState.DEFAULT}
                    />
                  </a>
                }
                popup={
                  <div>
                    If your Mac was made{" "}
                    <span className="font-semibold text-gray-300">
                      after November 2020
                    </span>
                    , it likely has an M1 chip. To confirm, click the{" "}
                    <FaApple className="inline-block relative bottom-0.5 text-gray-300" />{" "}
                    icon in the top left corner of your screen, select
                    &lsquo;About This Mac&lsquo;, and see the chip description.
                  </div>
                }
              />
            </div>
          </>
        ) : (
          <div className="flex justify-center">
            {" "}
            <WhistButton
              className="mt-12 mx-2"
              contents={<div className="flex">Join the Windows waitlist</div>}
              state={WhistButtonState.DEFAULT}
              onClick={() => setOpen(true)}
            />
          </div>
        )}
      </div>
    </div>
  )
}

export default Hero
