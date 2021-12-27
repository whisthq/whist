import React, { useState, useEffect } from "react";

import { omnibarContext } from "@app/renderer/context/omnibar";
import {
  Suggestion,
  newTabSuggestion,
  currentTabSuggestion,
  displayTabSuggestion,
} from "@app/renderer/components/omnibar/search/autosuggest/suggestions";

const AutoSuggest = () => {
  const context = omnibarContext();
  const [tabSuggestions, setTabSuggestions] = useState<
    Array<{ name: string; component: JSX.Element; id?: number }>
  >([]);

  const startingID = () => {
    if (context.suggestionHoverID < 5) return 0;
    if (context.suggestionHoverID > context.suggestions.length - 5)
      return context.suggestions.length - 5;
    return context.suggestionHoverID;
  };

  const onClick = (index: number) => {
    context.onNavigate({
      action: context.suggestions[context.suggestionHoverID]?.name,
      id: context.suggestions[context.suggestionHoverID]?.id,
      text: context.userInput,
    });
    context.setUserInput("");
    context.setHoverID(0);
  };

  useEffect(() => {
    const suggestions: Array<{ name: string; component: JSX.Element }> = [];
    context.ungroupedViews.forEach((view: { title: string; id: number }) => {
      if (view.title.toLowerCase().includes(context.userInput.toLowerCase()))
        suggestions.push(displayTabSuggestion(view.title, view.id));
    });
    setTabSuggestions(suggestions);
  }, [context.userInput, context.ungroupedViews]);

  useEffect(() => {
    if (context.ungroupedViews.length === 0) {
      context.setSuggestions([newTabSuggestion as any]);
    } else {
      context.setSuggestions([
        newTabSuggestion as any,
        currentTabSuggestion as any,
        ...tabSuggestions,
      ]);
    }
  }, [context.ungroupedViews, tabSuggestions]);

  return (
    <div className="w-full bg-gray-900 rounded p-3 max-h-72 overflow-y-scroll">
      {context.suggestions
        .slice(startingID())
        .map(
          (
            suggestion: { component: JSX.Element; name: string },
            index: number
          ) => (
            <div
              onMouseEnter={() =>
                context.setHoverID(context.suggestions.indexOf(suggestion))
              }
              key={context.suggestions.indexOf(suggestion)}
              onClick={() => onClick(index)}
            >
              <Suggestion
                text={suggestion.component}
                focused={
                  context.suggestionHoverID ===
                  context.suggestions.indexOf(suggestion)
                }
              />
            </div>
          )
        )}
    </div>
  );
};

export default AutoSuggest;
