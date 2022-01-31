import React, { useState, useEffect } from "react"
import classNames from "classnames"

import { WhistButton, WhistButtonState } from "@app/components/button"

const SelectBrowser = (props: {
  browsers: string[]
  onSelect: (browser: string) => void
}) => {
  return (
    <div className="w-full">
      <div className="font-semibold text-2xl text-gray-300">Import Tabs</div>
      <div className="m-auto mt-4 mb-4 text-gray-400 max-w-sm">
        Which browser would you like to import your tabs from?
      </div>
      <div className="mt-8">
        <div className="bg-gray-800 px-4 py-2 rounded w-96 m-auto">
          <select
            className="bg-gray-800 font-semibold text-gray-300 outline-none mt-1 block w-full py-2 text-base border-gray-300 sm:text-sm"
            defaultValue={props.browsers[0]}
            onChange={(evt: any) => {
              props.onSelect(evt.target.value)
            }}
          >
            {props.browsers.map((browser: string, index: number) => (
              <option key={index} value={browser}>
                {browser}
              </option>
            ))}
          </select>
        </div>
      </div>
    </div>
  )
}

const DownloadIcon = () => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    className="h-6 w-6 text-mint animate-pulse"
    viewBox="0 0 20 20"
    fill="currentColor"
  >
    <path
      fillRule="evenodd"
      d="M10.293 3.293a1 1 0 011.414 0l6 6a1 1 0 010 1.414l-6 6a1 1 0 01-1.414-1.414L14.586 11H3a1 1 0 110-2h11.586l-4.293-4.293a1 1 0 010-1.414z"
      clipRule="evenodd"
    />
  </svg>
)

const Importer = (props: {
  windows: any[] | undefined
  browsers: string[]
  onSubmitBrowser: (browser: string) => void
  onSubmitWindow: (urls: string[]) => void
}) => {
  const [browser, setBrowser] = useState<string>(props.browsers?.[0])
  const [processing, setProcessing] = useState(false)
  const [browserSelected, setBrowserSelected] = useState(false)
  const [selectedWindow, setSelectedWindow] = useState(1)

  const onSubmit = (browser: string) => {
    setProcessing(true)
    props.onSubmitBrowser(browser)
    setBrowserSelected(true)
  }

  const onSelect = (browser: string) => {
    setBrowser(browser)
  }

  const onSelectWindow = (id: number) => {
    setSelectedWindow(id)
  }

  const onKeyDown = (evt: any) => {
    if (evt.key === "Enter")
      props.onSubmitWindow(props.windows?.[selectedWindow - 1]?.urls)
  }

  useEffect(() => {
    if (props.windows !== undefined && props.windows.length === 1)
      props.onSubmitWindow(props.windows?.[selectedWindow - 1]?.urls)
  }, [props.windows])

  if (!browserSelected) {
    return (
      <div
        className={classNames(
          "flex flex-col h-screen items-center bg-gray-900",
          "justify-center font-body text-center px-12"
        )}
      >
        <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
        <DownloadIcon />
        <div className="mt-8">
          <SelectBrowser
            browsers={props.browsers}
            onSelect={(browser: string) => onSelect(browser)}
          />
        </div>
        <div>
          <WhistButton
            contents="Continue"
            className="mt-4 px-12 w-96 mx-auto py-2 text-gray-300 text-gray-900 bg-mint"
            state={
              processing
                ? WhistButtonState.PROCESSING
                : WhistButtonState.DEFAULT
            }
            onClick={() => onSubmit(browser)}
          />
        </div>
      </div>
    )
  } else {
    return (
      <div
        className={classNames(
          "flex flex-col h-screen items-center bg-gray-900",
          "justify-center font-body text-center px-12"
        )}
        onKeyDown={onKeyDown}
        tabIndex={0}
      >
        <div className="absolute top-0 left-0 w-full h-8 draggable"></div>
        <div className="font-semibold text-2xl text-gray-300 mt-8">
          Select Window
        </div>
        <div className="m-auto mt-2 mb-8 text-gray-400 max-w-sm ">
          Which of your currently open {browser} windows would you like to
          import tabs from?
        </div>
        <div className="flex space-x-6 max-w-lg overflow-x-auto flex-nowrap">
          {props.windows === undefined && (
            <div className="h-36">
              <div className="flex justify-center items-center mt-1 px-12 pt-6">
                <div className="animate-spin rounded-full h-8 w-8 border-t-2 border-b-2 border-gray-700"></div>
              </div>
            </div>
          )}
          {props.windows?.map((window, i) => (
            <button
              className={classNames(
                "rounded py-8 w-64 flex-shrink-0 relative outline-none cursor-pointer duration-100 h-36",
                selectedWindow === window.id ? "bg-blue" : "bg-gray-800"
              )}
              onClick={() => onSelectWindow(window.id)}
              key={i}
            >
              <div className="flex space-x-2 justify-center">
                <div>
                  <svg
                    xmlns="http://www.w3.org/2000/svg"
                    className={classNames(
                      "h-5 w-5 duration-100",
                      selectedWindow === window.id
                        ? "text-gray-300"
                        : "text-gray-500"
                    )}
                    viewBox="0 0 20 20"
                    fill="currentColor"
                  >
                    <path d="M7 9a2 2 0 012-2h6a2 2 0 012 2v6a2 2 0 01-2 2H9a2 2 0 01-2-2V9z" />
                    <path d="M5 3a2 2 0 00-2 2v6a2 2 0 002 2V5h8a2 2 0 00-2-2H5z" />
                  </svg>
                </div>
                <div
                  className={classNames(
                    selectedWindow === window.id
                      ? "text-gray-300"
                      : "text-gray-500"
                  )}
                >
                  {window.title.slice(0, 20)}
                  {window.title.length > 20 && "..."}
                </div>
              </div>
              <div
                className={classNames(
                  "text-gray-300 text-xs font-bold px-4 py-2 m-auto max-w-max mt-4 rounded duration-100",
                  selectedWindow === window.id ? "bg-indigo-500" : "bg-gray-700"
                )}
              >
                {window.urls.length} tab(s) open
              </div>
            </button>
          ))}
        </div>
        <button
          className="mt-12 text-gray-500 outline-none animate-fade-in-up opacity-0"
          onClick={() => {
            props.onSubmitWindow(props.windows?.[selectedWindow - 1]?.urls)
          }}
        >
          <div>
            Press{" "}
            <kbd className="bg-gray-700 rounded px-2 py-1 text-xs mx-1 text-gray-300">
              Enter
            </kbd>{" "}
            to import selected tabs
          </div>
        </button>
      </div>
    )
  }
}

export default Importer
