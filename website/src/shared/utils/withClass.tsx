import React from "react";
import classNames from "classnames";

/* eslint-disable react/display-name */

export const withClass =
  <T extends Function>(Element: T, ...classes: string[]) =>
  (props: any) =>
    <Element {...props} className={classNames(...classes, props.className)} />;
