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
      "text-md rounded bg-blue duration-500 focus:outline-none py-4 font-body font-semibold min-w-max transition-colors",
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
    "opacity-50 cursor-default"
  )

  const enabledClassName = classNames(
    props.className,
    "hover:bg-blue-light hover:text-gray"
  )

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
            <div className="flex justify-center items-center mt-1 px-12">
              <div className="animate-spin rounded-full h-5 w-5 border-t-2 border-b-2 border-gray-900"></div>
            </div>
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
