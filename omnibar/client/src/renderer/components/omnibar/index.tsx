import React, { useState } from "react";

import Tabs from "@app/renderer/components/omnibar/tabs";
import Search from "@app/renderer/components/omnibar/search";

import { omnibarContext } from "@app/renderer/context/omnibar";

const Footer = () => (
  <div className="absolute bottom-2 right-2 flex space-x-2 h-5">
    <kbd className="text-gray-500 text-xs mt-0.5 mr-1">To hide:</kbd>
    <kbd className="inline-flex items-center border border-gray-500 rounded px-2 text-xs font-medium text-gray-400">
      Option
    </kbd>
    <kbd className="inline-flex items-center border border-gray-500 rounded px-2 text-xs font-medium text-gray-400">
      A
    </kbd>
  </div>
);

const Omnibar = () => {
  /*
    This is the omnibar that the user summons via Option+F and functions as the user's center of command
  */
  const context = omnibarContext();
  const [viewFocusID, setViewFocusID] = useState(0);

  const onKeyDown = (e: any) => {
    if (e.key === "Enter" && context.userInput !== "") {
      context.onNavigate({
        action: context.suggestions[context.suggestionHoverID]?.name,
        id: context.suggestions[context.suggestionHoverID]?.id,
        text: context.userInput,
      });
      context.setUserInput("");
      context.setHoverID(0);
    }
  };

  return (
    <div
      tabIndex={0}
      onKeyDown={onKeyDown}
      className="w-screen h-screen text-gray-100 text-center bg-gray-900 bg-opacity-90 p-3 relative"
    >
      <Search />
      <Tabs />
    </div>
  );
};

export default Omnibar;
