import React from "react"
import classNames from "classnames"

export const BaseInput = (props: {
    value?: string;
    type?: string;
    className?: string;
    placeholder?: string;
    onChange?: (_: any) => void
}) => (
    <input type={props.type}
           className={classNames("text-lg rounded-md px-2 py-1.5 align-middle",
                                 "outline-none font-body",
                                 props.className)}
        value={props.value}
        placeholder={props.placeholder} onChange={props.onChange}>
    </input>
)
