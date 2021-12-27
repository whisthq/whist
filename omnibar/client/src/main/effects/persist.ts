import { net } from "electron";

import { persistSet } from "@app/utils/state/persist";
import { fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { emitCache } from "@app/utils/state/persist";

let bookmarked = [
  ["https://app.slack.com/client/TQ8RU2KE2/CQ6KN7N00"],
  ["https://www.github.com"],
  ["https://www.notion.so"],
  ["https://www.discord.com"],
  ["https://www.messenger.com"],
];

fromTrigger(FractalTrigger.appReady).subscribe(() => {
  bookmarked.forEach((urls: string[], index: number) => {
    const req = net.request(urls[0]);
    req.on("redirect", (_s, _m, redirectUrl, _r) => {
      bookmarked[index].push(redirectUrl);
      persistSet("bookmarked", bookmarked);
    });
    req.end();
  });
});

fromTrigger(FractalTrigger.appReady).subscribe(() => {
  emitCache();
});
