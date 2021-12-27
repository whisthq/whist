import { app } from "electron";
import { of } from "rxjs";
import path from "path";
import fse from "fs-extra";

import { fromTrigger, createTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { persistGet, persistSet } from "@app/utils/state/persist";
import { createOnboardingWindow } from "@app/utils/navigation/onboarding";

fromTrigger(FractalTrigger.appReady).subscribe(() => {
  const onboarded = persistGet("onboarded");
  if (!onboarded) {
    createOnboardingWindow();
  } else {
    createTrigger(FractalTrigger.doneOnboarding, of(undefined));
  }
});

fromTrigger(FractalTrigger.importChromeConfig).subscribe(() => {
  const appSupportPath = app.getPath("appData");
  const chromeConfigPath = path.join(appSupportPath, "/Google/Chrome/Default");
  const fractalParitionPath = path.join(
    appSupportPath,
    `/${app.isPackaged ? "FractalBar" : "Electron"}/Partitions/fractal`
  );

  fse.copySync(chromeConfigPath, fractalParitionPath, { overwrite: true });

  createTrigger(FractalTrigger.doneOnboarding, of(undefined));
});

fromTrigger(FractalTrigger.doneOnboarding).subscribe(() => {
  persistSet("onboarded", true);
});
