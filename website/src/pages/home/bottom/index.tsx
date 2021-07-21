import React from "react";
import { FaApple, FaWindows } from "react-icons/fa";

import {
  FractalButton,
  FractalButtonState,
} from "@app/pages/home/components/button";
import { ScreenSize } from "@app/shared/constants/screenSizes";
import { withContext } from "@app/shared/utils/context";

export const ActionPrompt = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  const { width } = withContext();

  return (
    <div className="mt-48 text-center">
      <div className="rounded px-0 md:px-12 py-10 md:py-14">
        <div className="text-4xl md:text-5xl tracking-wide leading-snug text-gray dark:text-gray-300 mb-4">
          Try the <span className="text-mint">Alpha Release.</span>
        </div>
        <div className="font-body text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-xl m-auto">
          Fractal is early, but we&lsquo;re excited to see what you&lsquo;ll do
          with a lighter, faster browser! Windows, Linux, and Android support
          are coming soon.
        </div>
        {width > ScreenSize.SMALL && (
          <div className="flex justify-center" id="download">
            <a
              href="https://fractal-chromium-macos-prod.s3.amazonaws.com/Fractal.dmg"
              download
            >
              <FractalButton
                className="mt-12 mx-2"
                contents={
                  <div className="flex">
                    <FaApple className="relative mr-3 top-0.5" />
                    macOS
                  </div>
                }
                state={FractalButtonState.DEFAULT}
              />
            </a>
            <FractalButton
              className="mt-12 mx-2"
              contents={
                <div className="flex">
                  <FaWindows className="relative mr-3 top-0.5" />
                  Coming Soon
                </div>
              }
              state={FractalButtonState.DISABLED}
            />
          </div>
        )}
      </div>
    </div>
  );
};

export default ActionPrompt;
