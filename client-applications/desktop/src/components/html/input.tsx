import React, { FC, ChangeEvent, KeyboardEvent } from "react"
import classNames from "classnames"

import FractalKey from "@app/@types/input"

/*
    Prop declarations
*/

export enum FractalInputState {
    DEFAULT,
    WARNING,
    SUCCESS,
}

interface BaseInputProps {
    value?: string
    type?: string
    placeholder?: string
    onChange?: (_: string) => void
    onEnterKey?: () => void
    className?: string
}

interface FractalInputProps extends BaseInputProps {
    state: FractalInputState
}

/*
    Components
*/

export const BaseInput: FC<BaseInputProps> = (props: BaseInputProps) => {
    /*
        Description:
            Base input component

        Arguments:
            value(string): Value of input component (what is typed in)
            type(string): Type of input (e.g. "text", "password")
            placeholder(string): Placeholder for input (e.g. "Enter your password")
            onChange((_: string) => void): Callback for when new text is typed
            onEnterKey(() =<> void): Callback for when enter key is pressed
            className(string): Optional additional CSS styling
    */

    const handleChange = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        if (props.onChange) {
            props.onChange(evt.target.value)
        }
    }

    // Handles the ENTER key
    const handleKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER && props.onEnterKey) {
            props.onEnterKey()
        }
    }

    return (
        <>
            <input
                type={props.type}
                className={classNames(
                    "text-md rounded px-4 py-4 align-middle w-full outline-none font-body border border-gray",
                    props.className
                )}
                value={props.value}
                placeholder={props.placeholder}
                onChange={handleChange}
                onKeyPress={handleKeyPress}
            ></input>
        </>
    )
}

export const FractalInput: FC<FractalInputProps> = (
    props: FractalInputProps
) => {
    /*
        Description:
            Base input component

        Arguments:
            value(string): Value of input component (what is typed in)
            type(string): Type of input (e.g. "text", "password")
            placeholder(string): Placeholder for input (e.g. "Enter your password")
            onChange((_: string) => void): Callback for when new text is typed
            onEnterKey(() =<> void): Callback for when enter key is pressed
            className(string): Optional additional CSS styling
            state(FractalInputState): State of input element (defaults to FractalInputState.DEFAULT)
    */

    const { state, ...baseInputProps } = props

    const warningClassName = classNames(props.className, "border border-red")
    const successClassName = classNames(
        props.className,
        "bg-green-50 border-green-50"
    )

    switch (state) {
        case FractalInputState.WARNING:
            const warningInputProps = Object.assign(baseInputProps, {
                className: warningClassName,
            })
            return <BaseInput {...warningInputProps} />
        case FractalInputState.SUCCESS:
            const successInputProps = Object.assign(baseInputProps, {
                className: successClassName,
            })
            return <BaseInput {...successInputProps} />
        case FractalInputState.DEFAULT:
            return <BaseInput {...baseInputProps} />
    }
}
