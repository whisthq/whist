import React, { useState, useEffect, createContext, useContext } from "react";
import { useMainState } from "@app/utils/state/ipc";
import { FractalTrigger } from "@app/utils/constants/triggers";

interface OnboardingContextInterface {
  configImportState: string;
  onConfigImport: () => void;
  onDoneOnboarding: () => void;
}

const OnboardingContext = createContext<OnboardingContextInterface>({
  configImportState: "",
  onConfigImport: () => {},
  onDoneOnboarding: () => {},
});

const OnboardingProvider = (props: { children: React.ReactNode }) => {
  const [mainState, setMainState] = useMainState();
  const [configImportState, setConfigImportState] = useState("");

  const onConfigImport = () => {
    setMainState({
      trigger: { name: FractalTrigger.importChromeConfig, payload: undefined },
    });
  };

  const onDoneOnboarding = () => {
    setMainState({
      trigger: { name: FractalTrigger.doneOnboarding, payload: undefined },
    });
  };

  useEffect(() => {}, []);

  return (
    <OnboardingContext.Provider
      value={{
        configImportState,
        onConfigImport,
        onDoneOnboarding,
      }}
    >
      {props.children}
    </OnboardingContext.Provider>
  );
};

const onboardingContext = () => {
  const context = useContext(OnboardingContext);
  if (context === undefined) {
    throw new Error("omnibarContext must be used within OmnibarProvider");
  }
  return context;
};

export { OnboardingProvider, onboardingContext };
