import React, { useEffect } from "react";
import className from "classnames";

import { omnibarContext, FractalView } from "@app/renderer/context/omnibar";
import Link from "@app/renderer/components/omnibar/tabs/link";
import Dock from "@app/renderer/components/omnibar/tabs/dock";
import Overlay from "@app/renderer/components/omnibar/tabs/overlay";

const Tabs = () => {
  const context = omnibarContext();

  return (
    <>
      <div
        className={className(
          "px-4 pt-6 pb-2 w-full grid grid-cols-8 overflow-y-scroll",
          context.showOverlay ? "opacity-0" : "opacity-100"
        )}
      >
        <Dock />
      </div>
      <Overlay
        contents={
          <div className="mt-4 overflow-y-scroll">
            {context?.groupedViews[context.groupFocusID]?.views?.map(
              (view: FractalView) => (
                <Link view={view} />
              )
            )}
          </div>
        }
      />
    </>
  );
};

export default Tabs;
