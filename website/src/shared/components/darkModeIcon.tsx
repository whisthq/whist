import React from 'react'
import classNames from 'classnames'
import { FaMoon, FaSun } from 'react-icons/fa'
import { withContext } from '@app/shared/utils/context'

export const DarkModeIcon = () => {
  const { dark, setDark } = withContext()

  return (
    <div className="absolute">
      <div
        onClick={() => setDark(!dark)}
        className={classNames(
          'fixed top-16 md:top-24 right-2 md:right-12 dark:bg-transparent',
          'border border-transparent dark:border-gray-400 bg-blue-lightest',
          'text-gray dark:text-gray-300 p-2 rounded cursor-pointer z-50'
        )}
      >
        <div>{dark ? <FaSun /> : <FaMoon />}</div>
      </div>
    </div>
  )
}
