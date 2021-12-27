import React from "react";
import className from "classnames";

import { omnibarContext } from "@app/renderer/context/omnibar";

const Link = (props: {
  view: {
    url: string;
    title: string;
    isOnTop: boolean;
    id: number;
  };
}) => {
  const onLinkClicked = () => {
    context.displayView?.(props.view?.id ?? -1, true);
    context.setShowOverlay(false);
  };

  const context = omnibarContext();
  return (
    <div
      className={className(
        "flex font-body relative p-3 rounded-md block w-full cursor-pointer hover:bg-opacity-60 text-left px-6 mt-2",
        props.view.isOnTop
          ? "bg-blue bg-opacity-90"
          : "bg-gray-700 bg-opacity-60"
      )}
    >
      <button
        className={className("truncate flex-grow text-left text-gray-300")}
        key={props.view?.id?.toString() ?? undefined}
        onClick={onLinkClicked}
      >
        {props.view.title}
      </button>
      {context.closeView !== undefined && (
        <button
          onClick={() => context.closeView?.(props.view?.id)}
          className="flex-none cursor-pointer z-30 text-gray-500 group-hover:opacity-100 hover:text-indigo-500 duration-75 pl-5"
        >
          <svg
            xmlns="http://www.w3.org/2000/svg"
            className="h-4 w-4"
            fill="none"
            viewBox="0 0 24 24"
            stroke="currentColor"
          >
            <path
              strokeLinecap="round"
              strokeLinejoin="round"
              strokeWidth="2"
              d="M6 18L18 6M6 6l12 12"
            />
          </svg>
        </button>
      )}
    </div>
  );
};

export default Link;
