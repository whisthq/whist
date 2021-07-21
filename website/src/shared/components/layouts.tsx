import React from "react";
import type { ReactNode } from "react";
import classNames from "classnames";

export const JustifyStartEndRow = (props: {
  className?: string;
  start?: ReactNode;
  middle?: ReactNode;
  end?: ReactNode;
}) => (
  <div className={classNames("flex w-full justify-between", props.className)}>
    {props.start ?? <div className="w-full"></div>}
    {props.middle ?? <div className="w-full"></div>}
    {props.end}
  </div>
);

export const JustifyStartEndCol = (props: {
  className?: string;
  start?: ReactNode;
  middle?: ReactNode;
  end?: ReactNode;
}) => (
  <div
    className={classNames(
      "flex flex-col w-full justify-between",
      props.className
    )}
  >
    {props.start ?? <div className="w-full h-full"></div>}
    {props.middle ?? <div className="w-full h-full"></div>}
    {props.end}
  </div>
);

export const ScreenFull = (props: {
  className?: string;
  children?: ReactNode | ReactNode[];
}) => (
  <div className={classNames("w-full h-screen", props.className)}>
    {props.children}
  </div>
);
