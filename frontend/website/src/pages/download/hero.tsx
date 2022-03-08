import React, { useState } from "react"
import { FaApple, FaWindows } from "react-icons/fa"
import { Link } from "react-scroll"

import { WhistButton, WhistButtonState } from "@app/shared/components/button"
import { ScreenSize } from "@app/shared/constants/screenSizes"
import { withContext } from "@app/shared/utils/context"
import { config } from "@app/shared/constants/config"
import Widget from "@app/pages/download/widget"

const AllowDownloads = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  const { width } = withContext()

  return (
    <div className="mb-24 text-center pt-36">
      <div className="px-0 md:px-12">
        <div className="text-6xl md:text-7xl tracking-wide leading-snug text-gray dark:text-gray-300 mb-4">
          Try the <span className="text-blue-light">alpha release</span>
        </div>
        <div className="text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-xl m-auto">
          Whist is early, but we&lsquo;re excited to see what you&lsquo;ll do
          with a lighter, faster Google Chrome. For optimal experience, please
          review the{" "}
          <Link to="requirements" spy={true} smooth={true}>
            <span className="text-blue-light cursor-pointer">
              requirements below
            </span>
          </Link>
          .
        </div>
        {width > ScreenSize.SMALL ? (
          <>
            {" "}
            <div className="flex justify-center">
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

const DontAllowDownloads = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  const [open, setOpen] = useState(false)

  return (
    <div className="mb-24 text-center pt-36">
      <div className="px-0 md:px-12">
        <Widget open={open} setOpen={setOpen} />
        <div className="text-6xl md:text-7xl tracking-wide leading-snug text-gray dark:text-gray-300 mb-4 max-w-4xl mx-auto">
          A <span className="text-blue-light">faster, lighter</span> browser
        </div>
        <div className="text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-3xl m-auto">
          Please fill out the survey below to see if you meet the requirements
          to download Whist. You will receive a download link at the end of the
          survey.
        </div>
        <div>
          <WhistButton
            className="mt-12 mx-2"
            contents={"Start Survey"}
            state={WhistButtonState.DEFAULT}
            onClick={() => setOpen(true)}
          />
        </div>
      </div>
    </div>
  )
}

const Hero = (props: { allowDownloads: boolean }) => {
  if (props.allowDownloads) return <AllowDownloads />
  return <DontAllowDownloads />
}

export default Hero
