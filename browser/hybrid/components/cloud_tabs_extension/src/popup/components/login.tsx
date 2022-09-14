import * as React from "react"

const Login = (props: { onClick: () => void }) => {
  return (
    <div className="min-h-full flex flex-col justify-center py-12 sm:px-6 lg:px-8">
      <div className="sm:mx-auto sm:w-full sm:max-w-md text-center">
        <h2 className="mt-2 text-center text-3xl font-extrabold text-gray-900 dark:text-gray-100">
          Sign in to Whist
        </h2>
        <p className="mt-2 text-center text-sm text-gray-700 dark:text-gray-500">
          To unlock cloud tabs
        </p>
        <button
          onClick={props.onClick}
          type="button"
          className="mt-6 inline-flex items-center px-6 py-2 border border-transparent text-base font-medium rounded-md shadow-sm text-gray-900 dark:text-gray-100 bg-gray-200 dark:bg-black dark:bg-opacity-60 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-light"
        >
          Sign In
        </button>
      </div>
    </div>
  )
}
export default Login
