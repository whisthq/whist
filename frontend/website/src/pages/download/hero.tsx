import React, { useState } from "react"
import { FaApple, FaWindows } from "react-icons/fa"

import Popover from "@app/shared/components/popover"
import { WhistButton, WhistButtonState } from "@app/shared/components/button"
import { ScreenSize } from "@app/shared/constants/screenSizes"
import { withContext } from "@app/shared/utils/context"
import { config } from "@app/shared/constants/config"
import Widget from "@app/pages/download/widget"

const Hero = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  const { width } = withContext()
  const [open, setOpen] = useState(false)

  return (
    <div className="mb-24 text-center pt-36">
      <div className="px-0 md:px-12">
        <Widget open={open} setOpen={setOpen} />
        <div className="text-6xl md:text-7xl tracking-wide leading-snug text-gray dark:text-gray-300 mb-4 max-w-4xl mx-auto">
          A <span className="text-blue-light">faster, lighter</span> browser
        </div>
        <div className="text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-3xl m-auto">
          In order to use Whist, you must have a{" "}
          <span className="text-blue-light">macOS 10.13+</span> computer and be{" "}
          <span className="text-blue-light">located in North America</span>. If
          you do not meet these requirements, please{" "}
          <span
            onClick={() => setOpen(true)}
            className="text-blue-light cursor-pointer"
          >
            leave us your email
          </span>{" "}
          and we will notify you when we can support you!
        </div>
        {width > ScreenSize.SMALL ? (
          <>
            {" "}
            <div className="flex justify-center">
              <Popover
                element={
                  <a href={config.client_download_urls.macOS_x64} download>
                    <WhistButton
                      className="mt-12 mx-2"
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
                      className="mt-12 mx-2"
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
              <WhistButton
                className="mt-12 mx-2"
                contents={
                  <div className="flex">
                    <FaWindows className="relative mr-3 top-0.5" />
                    Coming Soon
                  </div>
                }
                state={WhistButtonState.DISABLED}
              />
            </div>
          </>
        ) : (
          <div className="flex justify-center">
            {" "}
            <WhistButton
              className="mt-12 mx-2"
              contents={
                <div className="flex">
                  <FaApple className="relative mr-3 top-0.5" />
                  Mac Only
                </div>
              }
              state={WhistButtonState.DISABLED}
            />
          </div>
        )}
      </div>
    </div>
  )
}

export default Hero
