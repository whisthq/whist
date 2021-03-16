import React, { ChangeEvent, KeyboardEvent } from "react"
import classNames from "classnames"

export const BaseInput = (props: {
    value?: string;
    type?: string;
    className?: string;
    placeholder?: string;
    onChange?: (_: any) => void
}) => (
    <input type={props.type}
           className={classNames("text-md rounded px-4 py-4 align-middle",
                                 "outline-none font-body border border-gray",
                                 props.className)}
        value={props.value}
        placeholder={props.placeholder} onChange={props.onChange}>
    </input>
)

export const AuthInput = (props: {
    onChange?: (evt: ChangeEvent<HTMLInputElement>) => void
    onKeyPress?: (evt: KeyboardEvent) => void
    placeholder: string
    text?: string
    type: string
    valid?: boolean
    value?: string
    warning?: string
    className?: string
    altText?: JSX.Element
}) => {
    return (
        <div className={classNames("w-full", props.className)}>
            <div className="w-full flex justify-between">
                <div className="text-sm font-bold font-body text-gray">{props.text}</div>
                <div className="text-sm">{props.altText}</div>
                {props.warning && (
                    <div className="text-red bg-red-100 rounded text-sm px-6 py-2">
                        {props.warning}
                    </div>
                )}
            </div>
            <input
                type={props.type}
                placeholder={props.placeholder}
                onChange={props.onChange}
                onKeyPress={props.onKeyPress}
                value={props.value}
                className={classNames(
                    "text-md rounded px-4 py-4 align-middle mt-2 w-full",
                    "outline-none font-body border border-gray",
                    props.valid ? "bg-green-100" : "",
                    props.warning ? "border border-red" : ""
                )}
            />
        </div>
    )
}
