import React, { FC } from "react"
import classNames from "classnames"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

/*
    Prop declarations
*/

export enum FractalButtonState {
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

interface FractalButtonProps extends BaseButtonProps {
  state: FractalButtonState
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
      "text-md rounded bg-blue text-white duration-500 focus:outline-none py-4 font-body font-semibold",
      "transition-colors",
      props.className
    )}
    onClick={props.onClick}
    disabled={props.disabled}
  >
    {props.contents}
  </button>
)

export const FractalButton: FC<FractalButtonProps> = (
  props: FractalButtonProps
): JSX.Element => {
  /*
        Description:
            Returns a button component with custom text
        Arguments:
            contents(string|JSX.Element): Text to display, can be string or component
            onClick(() = void): Callback function for button click
            className(string): Optional additional Tailwind styling
            disabled(boolean): If true, button cannot be clicked
            state(FractalButtonType): Button state (defaults to FractalButtonState.DEFAULT)
    */

  const { state, ...baseButtonProps } = props

  const doNothing = () => {}

  const disabledClassName = classNames(
    props.className,
    "opacity-50 cursor-default"
  )

  const enabledClassName = classNames(
    props.className,
    "hover:bg-mint hover:text-gray"
  )

  switch (state) {
    case FractalButtonState.DISABLED: {
      const disabledButtonProps = Object.assign(baseButtonProps, {
        className: disabledClassName,
        onClick: doNothing,
      })
      return <BaseButton {...disabledButtonProps} />
    }
    case FractalButtonState.PROCESSING: {
      const processingButtonProps = Object.assign(baseButtonProps, {
        className: disabledClassName,
        contents: (
          <>
            <FontAwesomeIcon
              icon={faCircleNotch}
              spin
              className="text-white mt-1"
            />
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
