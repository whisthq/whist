import React from "react"
import classNames from "classnames"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

export const BaseButton = (props: {
    label: string
    className?: string
    placeholder?: string
    processing?: boolean
    onClick?: (_: any) => void
}): JSX.Element => (
    <button
        className={classNames(
            "text-md rounded bg-blue text-white duration-500 focus:outline-none py-4",
            "hover:bg-mint hover:text-gray",
            "transition-colors",
            props.className
        )}
        onClick={props.onClick}
    >
        {props.processing ? (
            <>
                <FontAwesomeIcon
                    icon={faCircleNotch}
                    spin
                    className="text-white"
                />
            </>
        ) : (
            <h5 className="transform font-body translate-y-0.5 font-semibold">
                {props.label}
            </h5>
        )}
    </button>
)
