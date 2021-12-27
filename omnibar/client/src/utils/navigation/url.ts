import validUrl from "valid-url";

import {
  getBrowserWindow,
  createBrowserWindow,
} from "@app/utils/navigation/browser";
import { createView, attachView } from "@app/utils/navigation/view";

export const sanitizeUrl = (raw: string) => {
  let output = null;

  const domain = /[a-z0-9-]+(\.[a-z]+)+/gi;
  const port = /(:[0-9]*)\w/g;

  if (domain.test(raw) || port.test(raw)) {
    if (raw.match(/^[a-zA-Z]+:\/\//) == null) {
      output = "https://www." + raw;
    } else {
      output = raw;
    }
  }

  return output ?? raw;
};

export const getSearchableUrl = (raw: string) => {
  const sanitized = sanitizeUrl(raw);
  const isURL = validUrl.isUri(sanitized);
  const finalUrl =
    isURL ?? false ? sanitized : `https://www.google.com/search?q=${sanitized}`;
  return finalUrl;
};

/* eslint-disable @typescript-eslint/strict-boolean-expressions */
export const navigate = (raw: string) => {
  const win = getBrowserWindow() ?? createBrowserWindow();
  const view = createView(getSearchableUrl(raw));
  attachView(view, win);
};
