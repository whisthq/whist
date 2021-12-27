import React from "react";

import { onboardingContext } from "@app/renderer/context/onboarding";

const Onboarding = () => {
  const context = onboardingContext();
  return (
    <div className="w-screen h-screen text-gray-100 bg-gray-900 p-6 relative text-left">
      <div className="px-4 py-5 sm:p-6">
        <div className="text-5xl text-gray-100 font-semibold">Welcome!</div>
        <div className="mt-2 max-w-xl text-md text-gray-400 mt-4">
          <p>
            Would you like to import your saved passwords from Google Chrome
            into Fractal?
          </p>
        </div>
        <div className="mt-44 flex space-x-4">
          <button
            onClick={context.onConfigImport}
            type="button"
            className="font-semibold inline-flex items-center justify-center px-4 py-2 border border-transparent font-medium rounded-md text-green-600 bg-green-100 hover:bg-green-200 focus:outline-none sm:text-sm"
          >
            Yes, import
          </button>
          <button
            onClick={context.onDoneOnboarding}
            type="button"
            className="inline-flex items-center justify-center px-4 py-2 border border-transparent font-medium rounded-md text-blue-700 bg-blue-100 hover:bg-blue-200 focus:outline-none sm:text-sm"
          >
            No, start from a clean slate
          </button>
        </div>
      </div>
    </div>
  );
};

export default Onboarding;
