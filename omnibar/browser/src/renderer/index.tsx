/**
 * Copyright Fractal Computers, Inc. 2021
 * @file index.tsx
 * @brief This file is the entry point of the renderer thread and acts as a router.
 */

import React from "react";
import { chain } from "lodash";
import ReactDOM from "react-dom";

import Omnibar from "@app/renderer/components/omnibar";
import Finder from "@app/renderer/components/finder";
import Onboarding from "@app/renderer/components/onboarding";
import {
  WindowHashOmnibar,
  WindowHashFinder,
  WindowHashOnboarding,
} from "@app/utils/constants/app";
import { OmnibarProvider } from "@app/renderer/context/omnibar";
import { FinderProvider } from "@app/renderer/context/finder";
import { OnboardingProvider } from "@app/renderer/context/onboarding";

// Electron has no way to pass data to a newly launched browser
// window. To avoid having to maintain multiple .html files for
// each kind of window, we pass a constant across to the renderer
// thread as a query parameter to determine which component
// should be rendered.

// If no query parameter match is found, we default to a
// generic navigation error window.
const show = chain(window.location.search.substring(1))
  .split("=")
  .chunk(2)
  .fromPairs()
  .get("show")
  .value();

const RootComponent = () => {
  if (show === WindowHashOmnibar)
    return (
      <OmnibarProvider>
        <Omnibar />
      </OmnibarProvider>
    );
  if (show === WindowHashFinder)
    return (
      <FinderProvider>
        <Finder />
      </FinderProvider>
    );
  if (show === WindowHashOnboarding)
    return (
      <OnboardingProvider>
        <Onboarding />
      </OnboardingProvider>
    );
  return (
    <div className="bg-gray-900 text-gray-300 w-screen h-screen text-center pt-36">
      Woops! You've found an unidentified bug. Please contact
      support@fractal.co.
    </div>
  );
};

ReactDOM.render(
  <div className="w-screen h-screen font-body overflow-y-hidden">
    <RootComponent />
  </div>,
  document.getElementById("root")
);
