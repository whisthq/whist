import React from "react"
import classNames from "classnames"

export const BaseButton = (props: {
    label: string
    className?: string;
    placeholder?: string;
    onClick?: (_: any) => void
}): JSX.Element => (
    <button className={classNames("text-lg h-10 rounded-md focus:outline-none",
                                  "hover:bg-gray-500", "transition-colors",
                                  props.className)}
        onClick={props.onClick}>
        <h5 className="transform font-body translate-y-0.5">{props.label}</h5>
    </button>
)
