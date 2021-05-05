import React from 'react'
import classNames from 'classnames'
import { FaMoon, FaSun } from 'react-icons/fa'

export const DarkModeIcon = (props: { dark: boolean, onClick: () => void }) => (
    <div className="absolute">
      <div
        onClick={props.onClick}
        className={classNames(
          'fixed top-16 md:top-24 right-2 md:right-12 dark:bg-transparent',
          'border border-transparent dark:border-gray-400 bg-blue-lightest',
          'text-gray dark:text-gray-300 p-2 rounded cursor-pointer z-50'
        )}
      >
        <div>{props.dark ? <FaSun /> : <FaMoon />}</div>
      </div>
    </div>
  )