/**
 * Copyright Fractal Computers, Inc. 2021
 * @file index.tsx
 * @brief This file is the entry point of the renderer thread and acts as a router.
 */

import React from "react";
import ReactDOM from "react-dom";

const RootComponent = () => {
  return (
    <div className="bg-gray-900 text-gray-300 w-screen h-screen text-center pt-36">
      Woops! You've found an unidentified bug. Please contact
      support@whist.com.
    </div>
  );
};

ReactDOM.render(
  <div className="w-screen h-screen font-body overflow-y-hidden">
    <RootComponent />
  </div>,
  document.getElementById("root")
);
