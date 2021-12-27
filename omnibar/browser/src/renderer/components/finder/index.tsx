import React, { useState } from "react";

import { finderContext } from "@app/renderer/context/finder";

const Finder = () => {
  /*
    This is the omnibar that the user summons via Option+F and acts as the user's center of command
  */
  const [text, setText] = useState("");
  const context = finderContext();

  const onChange = (e: any) => {
    setText(e.target.value);
  };

  const onKeyDown = (e: any) => {
    if (e.key === "Enter" && text !== "") {
      context.onFinderSearch(text);
    }
  };

  return (
    <div className="w-screen h-screen bg-none bg-opacity-0 flex justify-between bg-gray-900 bg-opacity-90 px-4">
      <input
        autoFocus
        className="w-48 h-full placeholder-gray-500 bg-transparent text-gray-300"
        placeholder="Search this page"
        value={text}
        onKeyDown={onKeyDown}
        onChange={onChange}
      />
      <div
        className="mt-6 text-blue cursor-pointer"
        onClick={() => context.onFinderUp(text)}
      >
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-5 w-5"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M9 11l3-3m0 0l3 3m-3-3v8m0-13a9 9 0 110 18 9 9 0 010-18z"
          />
        </svg>
      </div>
      <div
        className="mt-6 text-blue cursor-pointer"
        onClick={() => context.onFinderDown(text)}
      >
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-5 w-5"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M15 13l-3 3m0 0l-3-3m3 3V8m0 13a9 9 0 110-18 9 9 0 010 18z"
          />
        </svg>
      </div>
      <div
        className="mt-6 text-blue cursor-pointer"
        onClick={context.onFinderClose}
      >
        <svg
          xmlns="http://www.w3.org/2000/svg"
          className="h-5 w-5"
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth="2"
            d="M10 14l2-2m0 0l2-2m-2 2l-2-2m2 2l2 2m7-2a9 9 0 11-18 0 9 9 0 0118 0z"
          />
        </svg>
      </div>
    </div>
  );
};

export default Finder;
