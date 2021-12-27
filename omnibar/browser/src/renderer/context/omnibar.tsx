import React, { useState, useEffect, createContext, useContext } from "react";
import { concat, some, method, findIndex } from "lodash";

import { useMainState } from "@app/utils/state/ipc";
import { FractalTrigger } from "@app/utils/constants/triggers";

const getHost = (raw: string) => {
  try {
    const urlObj = new URL(raw);
    const host = urlObj.hostname?.replace("www.", "");
    return host;
  } catch {
    return raw;
  }
};

const getImage = (raw: string) => {
  try {
    if (raw.includes("www.messenger.com"))
      return "https://facebookbrand.com/wp-content/uploads/2020/10/Logo_Messenger_NewBlurple-399x399-1.png?w=399&h=399";
    if (raw.includes("www.gmail.com"))
      return "https://upload.wikimedia.org/wikipedia/commons/thumb/7/7e/Gmail_icon_%282020%29.svg/1024px-Gmail_icon_%282020%29.svg.png";
    const urlObj = new URL(raw);
    const image = `https://logo.clearbit.com/${urlObj.hostname}`;
    return image;
  } catch (err) {
    console.error(err);
    return "";
  }
};

interface FractalView {
  url: string;
  title: string;
  isOnTop: boolean;
  id: number;
  image?: string;
}

interface OmnibarContextInterface {
  groupedViews: Array<{ urls: string[]; image: string; views: FractalView[] }>; // All open views passed from main thread, grouped by hostname
  groupFocusID: number;
  ungroupedViews: FractalView[]; // All open views, ungrouped
  topView?: FractalView;
  userInput: string; // Text that the user has typed into the search bar
  showOverlay: boolean; // Whether to show the grouped tabs overlay
  suggestionHoverID: number; // Used to track which autosuggestion is in focus
  suggestions: Array<{ name: string; component: JSX.Element; id?: number }>; // Used to track suggestions
  setSuggestions: (
    suggestions: Array<{ name: string; component: JSX.Element; id?: number }>
  ) => void;
  setGroupFocusID: (id: number) => void;
  setHoverID: (id: number) => void;
  setUserInput: (userInput: string) => void;
  setShowOverlay: (showOverlay: boolean) => void;
  changeViewUrl: (text: string, id: number) => void;
  createView: (text: string) => void;
  displayView: (id: number, hideOmnibar: boolean) => void;
  closeView: (id: number) => void;
  onNavigate: (args: { action: string; id?: number; text?: string }) => void;
  setTopView: (view: FractalView) => void;
}

const OmnibarContext = createContext<OmnibarContextInterface>({
  groupedViews: [],
  groupFocusID: 0,
  ungroupedViews: [],
  topView: undefined,
  userInput: "",
  showOverlay: false,
  suggestionHoverID: 0,
  suggestions: [],
  setSuggestions: () => {},
  setGroupFocusID: () => {},
  setHoverID: () => {},
  setUserInput: () => {},
  setShowOverlay: () => {},
  createView: () => {},
  displayView: () => {},
  closeView: () => {},
  onNavigate: () => {},
  changeViewUrl: () => {},
  setTopView: () => {},
});

const OmnibarProvider = (props: { children: React.ReactNode }) => {
  const [mainState, setMainState] = useMainState();
  const [groupedViews, setGroupedViews] = useState<
    Array<{ urls: string[]; image: string; views: FractalView[] }>
  >([]);
  const [ungroupedViews, setUngroupedViews] = useState<FractalView[]>([]);
  const [topView, setTopView] = useState<FractalView | undefined>(undefined);
  const [userInput, setUserInput] = useState("");
  const [showOverlay, setShowOverlay] = useState(false);
  const [suggestionHoverID, setHoverID] = useState(0);
  const [suggestions, setSuggestions] = useState<
    Array<{ name: string; component: JSX.Element }>
  >([]);
  const [groupFocusID, setGroupFocusID] = useState(0);

  useEffect(() => {
    const views = mainState?.trigger?.payload?.views;

    let grouped: Array<{
      urls: string[];
      image: string;
      views: FractalView[];
    }> =
      mainState?.persisted?.bookmarked?.map((urls: string[]) => ({
        urls,
        image: getImage(urls[0]),
        views: [],
      })) ?? [];

    grouped = concat(
      [
        {
          urls: [],
          image: "",
          views: [],
        },
      ],
      grouped
    );

    views?.forEach((view: FractalView) => {
      const host = getHost(view.url);
      let groupIndex = Math.max(
        findIndex(grouped, (x) => some(x.urls, method("includes", host))),
        0
      );
      if (view.url.startsWith("https://www.google.com/search?q="))
        groupIndex = 0;
      grouped[groupIndex] = {
        ...grouped[groupIndex],
        views: [...grouped[groupIndex].views, { ...view }],
      };
      if (view.isOnTop) {
        setTopView(view);
        setGroupFocusID(groupIndex);
      }
    });
    setGroupedViews(grouped);
  }, [mainState]);

  useEffect(() => {
    const ungrouped: FractalView[] = [];
    groupedViews.forEach(
      (x: { urls: string[]; image: string; views: FractalView[] }) => {
        x.views.forEach((view: FractalView) => {
          ungrouped.push(view);
        });
      }
    );
    if (ungrouped.length === 0) setTopView(undefined);
    setUngroupedViews(ungrouped);
  }, [groupedViews]);

  useEffect(() => {
    if (userInput === "") setHoverID(0);
  }, [userInput]);

  const createView = (text: string) => {
    setMainState({
      trigger: {
        name: FractalTrigger.createView,
        payload: { text },
      },
    });
  };

  const displayView = (id: number, hideOmnibar: boolean) => {
    setMainState({
      trigger: {
        name: FractalTrigger.displayView,
        payload: { id, hideOmnibar },
      },
    });
  };

  const closeView = (id: number) => {
    setMainState({
      trigger: { name: FractalTrigger.closeView, payload: { id } },
    });
  };

  const changeViewUrl = (text: string, id: number) => {
    setMainState({
      trigger: { name: FractalTrigger.changeViewUrl, payload: { text, id } },
    });
  };

  const onNavigate = (args: { action: string; id?: number; text?: string }) => {
    switch (args.action) {
      case "OPEN_IN_NEW_TAB": {
        createView(args.text ?? "");
        break;
      }
      case "OPEN_IN_CURRENT_TAB": {
        changeViewUrl(args.text ?? "", topView?.id ?? -1);
        break;
      }
      case "DISPLAY_TAB": {
        displayView(args.id ?? -1, false);
      }
    }
  };

  return (
    <OmnibarContext.Provider
      value={{
        groupedViews,
        groupFocusID,
        ungroupedViews,
        topView,
        userInput,
        showOverlay,
        suggestionHoverID,
        suggestions,
        setSuggestions,
        setGroupFocusID,
        setHoverID,
        setUserInput,
        setShowOverlay,
        createView,
        displayView,
        closeView,
        onNavigate,
        changeViewUrl,
        setTopView,
      }}
    >
      {props.children}
    </OmnibarContext.Provider>
  );
};

const omnibarContext = () => {
  const context = useContext(OmnibarContext);
  if (context === undefined) {
    throw new Error("omnibarContext must be used within OmnibarProvider");
  }
  return context;
};

export { OmnibarProvider, omnibarContext, FractalView };
