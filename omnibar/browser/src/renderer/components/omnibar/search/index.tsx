import React, { useEffect } from "react";

import AutoSuggest from "@app/renderer/components/omnibar/search/autosuggest";
import { omnibarContext } from "@app/renderer/context/omnibar";

const Search = () => {
  const context = omnibarContext();

  const onChange = (e: any) => {
    context.setUserInput(e.target.value);
  };

  const onKeyDown = (e: any) => {
    if (e.key === "ArrowDown" && context.userInput !== "") {
      e.preventDefault();
      if (context.suggestionHoverID === context.suggestions.length - 1) {
        context.setHoverID(0);
      } else {
        context.setHoverID(
          Math.min(
            context.suggestionHoverID + 1,
            context.suggestions.length - 1
          )
        );
      }
    } else if (e.key === "ArrowUp" && context.userInput !== "") {
      e.preventDefault();
      if (context.suggestionHoverID === 0) {
        context.setHoverID(context.suggestions.length - 1);
      } else {
        context.setHoverID(Math.max(context.suggestionHoverID - 1, 0));
      }
    }
  };

  useEffect(() => {
    if (context.userInput === "") {
      context.setHoverID(0);
    }
  }, [context.userInput]);

  return (
    <div className="relative py-4 border border-gray-600 border-t-0 border-l-0 border-r-0">
      <div className="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none bg-none">
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-4 w-4 text-gray-500"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"
          />
        </svg>
      </div>
      <input
        className="block w-full pl-10 sm:text-sm rounded-md bg-transparent pt-1"
        autoFocus
        placeholder="Search for a website, tab, or Google"
        value={context.userInput}
        onKeyDown={onKeyDown}
        onChange={onChange}
      />
      {context.userInput !== "" && (
        <div className="absolute w-full top-16 z-40">
          <AutoSuggest />
        </div>
      )}
    </div>
  );
};

export default Search;
