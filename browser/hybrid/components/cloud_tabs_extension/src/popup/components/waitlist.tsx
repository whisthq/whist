import * as React from "react"
import classNames from "classnames"
import { useState, useEffect } from "react"

import Logo from "../assets/whist_logo.png"

const Waitlist = (props: {
  onClick: (code: string) => void
  error: boolean
}) => {
  const [code, setCode] = useState("")
  const [showError, setShowError] = useState(false)

  useEffect(() => {
    setShowError(props.error)
  }, [props.error])

  const onChange = (e: any) => {
    setCode(e.target.value)
    setShowError(false)
    console.log("changing to", e.target.value)
  }

  const onKeyDown = (e: any) => {
    console.log("key down", code)
    if (e.key === "Enter") {
      props.onClick(code)
      setShowError(false)
    }
  }

  return (
    <>
      <div className="min-h-full flex items-center justify-center py-8 px-12">
        <div className="max-w-md w-full space-y-8">
          <div>
            <img className="mx-auto h-16 w-16" src={Logo} />
            <h2 className="dark:text-gray-100 mt-6 text-center text-2xl font-extrabold text-gray-900">
              Enter your invite code
            </h2>
            <p className="mt-1 text-center text-sm text-gray-600 dark:text-gray-300">
              To unlock cloud tabs.{" "}
              <a
                href="https://whist.typeform.com/to/EScjxr3N"
                target="_blank"
                className="text-blue"
                rel="noreferrer"
              >
                Need an invite code?
              </a>
            </p>
          </div>
          <div className="mt-4 space-y-1">
            <div
              className={classNames(
                "flex border justify-between rounded shadow-sm -space-y-px flex bg-gray-300 dark:bg-black bg-opacity-20 hover:bg-opacity-30 dark:text-gray-300 rounded focus:outline-none focus:z-10 text-sm border",
                showError
                  ? "border-red-400"
                  : "dark:border-gray-700 border-gray-300"
              )}
            >
              <input
                id="invite-code"
                name="invite-code"
                type="text"
                required
                className={classNames(
                  "grow appearance-none relative block px-3 py-3 placeholder-gray-500 text-gray-900 dark:text-gray-300 bg-transparent outline-none"
                )}
                placeholder="Invite Code"
                value={code}
                onChange={onChange}
                onKeyDown={onKeyDown}
              />
              <button
                type="submit"
                className={classNames(
                  "text-blue group relative py-3 pr-4 bg-none text-right duration-150 font-semibold",
                  code.length > 1 ? "visible" : "invisible"
                )}
                onClick={() => props.onClick(code)}
              >
                Continue
              </button>
            </div>
            <div
              className={classNames(
                "text-red-400 duration-150",
                showError ? "opacity-100" : "opacity-0"
              )}
            >
              Invalid code
            </div>
          </div>
        </div>
      </div>
    </>
  )
}

export default Waitlist
