import React from "react";
import className from "classnames";

import { omnibarContext, FractalView } from "@app/renderer/context/omnibar";

const Dock = () => {
  const context = omnibarContext();

  const onClick = (index: number) => {
    if (context.groupedViews[index]?.views.length > 0) {
      context.displayView(context.groupedViews[index].views[0].id, false);
      context.setGroupFocusID(index);
      if (context.groupedViews[index]?.views.length > 1)
        context.setShowOverlay(true);
    } else {
      context.createView(context.groupedViews[index].urls[0]);
    }
  };

  return (
    <>
      {context?.groupedViews?.map(
        (x: { image: string; views: FractalView[] }, index: number) => {
          return (
            <div className="flex flex-col text-center">
              <button
                className={className(
                  "h-12 w-12 bg-white p-1.5 rounded rounded cursor-pointer",
                  context.groupFocusID === index
                    ? "bg-opacity-100 text-blue bg-white"
                    : "bg-opacity-30"
                )}
                onClick={() => onClick(index)}
              >
                {(x.image ?? "") === "" ? (
                  <svg
                    xmlns="http://www.w3.org/2000/svg"
                    className="h-5 w-5 relative right-1 top-1"
                    fill="none"
                    viewBox="0 0 24 24"
                    stroke="currentColor"
                  >
                    <path
                      stroke-linecap="round"
                      stroke-linejoin="round"
                      stroke-width="2"
                      d="M12 5v.01M12 12v.01M12 19v.01M12 6a1 1 0 110-2 1 1 0 010 2zm0 7a1 1 0 110-2 1 1 0 010 2zm0 7a1 1 0 110-2 1 1 0 010 2z"
                    />
                  </svg>
                ) : (
                  <div className="rounded bg-white w-full h-full p-1">
                    <img src={x.image ?? ""} />
                  </div>
                )}
              </button>
              <div
                className={className(
                  "w-1.5 h-1.5 rounded-full bg-gray-400 mt-2 ml-5",
                  x.views.length > 0 ? "bg-opacity-100" : "bg-opacity-0"
                )}
              ></div>
            </div>
          );
        }
      )}
    </>
  );
};

export default Dock;
