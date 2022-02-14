import React, { FC } from "react"
import classNames from "classnames"

/*
    Prop declarations
*/

export enum WhistButtonState {
  DEFAULT,
  DISABLED,
  PROCESSING,
}

interface BaseButtonProps {
  contents: JSX.Element | string
  onClick?: (_: any) => void
  className?: string
  disabled?: boolean
}

interface WhistButtonProps extends BaseButtonProps {
  state: WhistButtonState
}

/*
    Components
*/

const BaseButton: FC<BaseButtonProps> = (props: BaseButtonProps) => (
  /*
        Description:
            Base button component with custom text

        Arguments:
            contents(string|JSX.Element): Text to display, can be string or component
            onClick(() = void): Callback function for button click
            className(string): Optional additional Tailwind styling
            disabled(boolean): If true, button cannot be clicked
    */

  <button
    className={classNames(
      "text-md rounded text-gray-900 duration-500 focus:outline-none py-3 px-12 font-body tracking-wide font-semibold bg-mint",
      "transition-colors",
      props.className
    )}
    onClick={props.onClick}
    disabled={props.disabled}
  >
    {props.contents}
  </button>
)

export const WhistButton: FC<WhistButtonProps> = (
  props: WhistButtonProps
): JSX.Element => {
  /*
        Description:
            Returns a button component with custom text

        Arguments:
            contents(string|JSX.Element): Text to display, can be string or component
            onClick(() = void): Callback function for button click
            className(string): Optional additional Tailwind styling
            disabled(boolean): If true, button cannot be clicked
            state(WhistButtonType): Button state (defaults to WhistButtonState.DEFAULT)
    */

  const { state, ...baseButtonProps } = props

  const doNothing = () => {}

  const disabledClassName = classNames(
    props.className,
    "opacity-30 pointer-events-none"
  )

  const enabledClassName = classNames(props.className)

  switch (state) {
    case WhistButtonState.DISABLED: {
      const disabledButtonProps = Object.assign(baseButtonProps, {
        className: disabledClassName,
        onClick: doNothing,
      })
      return <BaseButton {...disabledButtonProps} />
    }
    case WhistButtonState.PROCESSING: {
      const processingButtonProps = Object.assign(baseButtonProps, {
        className: disabledClassName,
        contents: (
          <>
            <svg
              className="animate-spin -ml-1 mr-3 h-5 w-5 text-white"
              xmlns="http://www.w3.org/2000/svg"
              fill="none"
              viewBox="0 0 24 24"
            >
              <circle
                className="opacity-25"
                cx="12"
                cy="12"
                r="10"
                stroke="currentColor"
                strokeWidth="4"
              ></circle>
              <path
                className="opacity-75"
                fill="currentColor"
                d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"
              ></path>
            </svg>
          </>
        ),
        onClick: doNothing,
      })
      return <BaseButton {...processingButtonProps} />
    }
    default: {
      const enabledButtonProps = Object.assign(baseButtonProps, {
        className: enabledClassName,
      })
      return <BaseButton {...enabledButtonProps} />
    }
  }
}
