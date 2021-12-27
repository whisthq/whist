import React from "react";
import className from "classnames";

import { omnibarContext } from "@app/renderer/context/omnibar";

const Suggestion = (props: {
  text: JSX.Element;
  focused: boolean;
  shortcuts?: string[];
}) => {
  return (
    <div
      className={className(
        "group flex justify-between w-full px-4 py-2 hover:text-gray-300 cursor-pointer rounded",
        props.focused ? "bg-gray-700" : ""
      )}
    >
      <div
        className={className(
          "text-sm",
          props.focused ? "text-gray-300" : "text-gray-500"
        )}
      >
        {props.text}
      </div>
      <div className="flex space-x-2">
        {props.shortcuts?.map((shortcut: string) => (
          <>
            <kbd className="inline-flex items-center border border-gray-500 rounded px-2 text-xs font-medium text-gray-500">
              {shortcut}
            </kbd>
          </>
        ))}
      </div>
    </div>
  );
};

const NewTabSuggestion = () => {
  const context = omnibarContext();
  return (
    <div className="flex space-x-2 pt-1.5">
      <div>Open</div>
      <div className="text-gray-100 bg-blue rounded text-xs font-semibold py-1.5 px-3 max-w-lg truncate relative bottom-1">
        {context.userInput}
      </div>
      <div>in new tab</div>
    </div>
  );
};

const CurrentTabSuggestion = () => {
  const context = omnibarContext();

  return (
    <div className="flex space-x-2 pt-1.5">
      <div>Open</div>
      <div className="text-gray-100 bg-blue rounded text-xs font-semibold py-1.5 px-3 max-w-lg truncate relative bottom-1">
        {context.userInput}
      </div>
      <div>in current tab</div>
    </div>
  );
};

const DisplayTabSuggestion = (props: { title: string }) => {
  return (
    <div tabIndex={0} className="flex space-x-2 pt-1.5">
      <div>Go to</div>
      <div className="text-gray-900 bg-mint rounded text-xs font-semibold py-1.5 px-3 max-w-sm truncate relative bottom-1">
        {props.title}
      </div>
      <div>which is currently open</div>
    </div>
  );
};

const newTabSuggestion = {
  name: "OPEN_IN_NEW_TAB",
  component: <NewTabSuggestion />,
};

const currentTabSuggestion = {
  name: "OPEN_IN_CURRENT_TAB",
  component: <CurrentTabSuggestion />,
};

const displayTabSuggestion = (title: string, id: number) => ({
  name: "DISPLAY_TAB",
  component: <DisplayTabSuggestion title={title} />,
  id: id,
});

export {
  Suggestion,
  newTabSuggestion,
  currentTabSuggestion,
  displayTabSuggestion,
};
