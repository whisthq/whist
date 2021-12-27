import React, { createContext, useContext } from "react";
import { useMainState } from "@app/utils/state/ipc";
import { FractalTrigger } from "@app/utils/constants/triggers";

interface FinderContextInterface {
  onFinderSearch: (text: string) => void;
  onFinderDown: (text: string) => void;
  onFinderUp: (text: string) => void;
  onFinderClose: () => void;
}

const FinderContext = createContext<FinderContextInterface>({
  onFinderSearch: () => {},
  onFinderDown: () => {},
  onFinderUp: () => {},
  onFinderClose: () => {},
});

const FinderProvider = (props: { children: React.ReactNode }) => {
  const [, setMainState] = useMainState();

  const onFinderSearch = (text: string) => {
    setMainState({
      trigger: { name: FractalTrigger.finderEnterPressed, payload: { text } },
    });
  };

  const onFinderDown = (text: string) => {
    setMainState({
      trigger: { name: FractalTrigger.finderDown, payload: { text } },
    });
  };

  const onFinderUp = (text: string) => {
    setMainState({
      trigger: { name: FractalTrigger.finderUp, payload: { text } },
    });
  };

  const onFinderClose = () => {
    setMainState({
      trigger: { name: FractalTrigger.finderClose, payload: undefined },
    });
  };

  return (
    <FinderContext.Provider
      value={{
        onFinderSearch,
        onFinderDown,
        onFinderUp,
        onFinderClose,
      }}
    >
      {props.children}
    </FinderContext.Provider>
  );
};

const finderContext = () => {
  const context = useContext(FinderContext);
  if (context === undefined) {
    throw new Error("finderContext must be used within FinderProvider");
  }
  return context;
};

export { FinderProvider, finderContext };
