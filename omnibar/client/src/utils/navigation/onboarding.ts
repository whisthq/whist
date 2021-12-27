import { screen } from "electron";

import {
  createWindow,
  base,
  getWindow,
  width,
  height,
} from "@app/utils/navigation";
import { WindowHashOnboarding } from "@app/utils/constants/app";

export const createOnboardingWindow = () => {
  /*
    Create the primary "base" browser window

    Args:
        none
    Returns:
        win: BrowserWindow
   */
  const win = createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.sm,
      frame: false, // Frameless, no traffic light controls or titlebar
      resizable: false,
      fullscreenable: false,
      minimizable: false,
      titleBarStyle: "customButtonsOnHover",
      id: WindowHashOnboarding
    },
    hash: WindowHashOnboarding,
    withReact: true,
  });

  return win;
};

export const getOnboardingWindow = () => getWindow(WindowHashOnboarding);
