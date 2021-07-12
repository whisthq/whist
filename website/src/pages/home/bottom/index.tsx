import React from "react";
import { FaApple } from "react-icons/fa";

import {
  FractalButton,
  FractalButtonState,
} from "@app/pages/home/components/button";

export const ActionPrompt = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  return (
    <div className="mt-48 text-center">
      <div className="rounded px-0 md:px-12 py-10 md:py-14">
        <div className="text-4xl md:text-5xl tracking-wide leading-snug text-gray dark:text-gray-300 mb-4">
          Try the <span className="text-mint">Alpha Release.</span>
        </div>
        <div className="font-body text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-xl m-auto">
          Fractal is early, but we're excited to see what you'll do with a lighter, faster browser! Windows, Linux,
          and Android support are coming soon.
        </div>
        <FractalButton
          className="mt-12"
          contents={<div className="flex"><FaApple className="relative mr-3 top-0.5"/>macOS</div>}
          state={FractalButtonState.DEFAULT}
        />
      </div>
    </div>
  );
};

export default ActionPrompt;
