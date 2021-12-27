import { app } from "electron";
import AutoLaunch from "auto-launch";

import { fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";

fromTrigger(FractalTrigger.appReady).subscribe(() => {
  const fractalAutoLaunch = new AutoLaunch({
    name: "FractalBar",
    path: "/Applications/FractalBar.app",
  });

  fractalAutoLaunch
    .isEnabled()
    .then((enabled: boolean) => {
      if (!enabled) fractalAutoLaunch.enable();
    })
    .catch((err) => console.error(err));
});

// fromTrigger(FractalTrigger.appReady).subscribe(() => {
//   if (app.isPackaged) {
//     app.setAsDefaultProtocolClient("http");
//     app.setAsDefaultProtocolClient("https");
//   }
// });
