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
           className={classNames("text-md rounded px-4 py-4 align-middle",
                                 "outline-none font-body border border-gray",
                                 props.className)}
        value={props.value}
        placeholder={props.placeholder} onChange={props.onChange}>
    </input>
)
