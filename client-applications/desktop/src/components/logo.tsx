import React from "react"
import classNames from "classnames"
import LogoBlack from "@app/assets/icons/logoBlack.svg"
import LogoWhite from "@app/assets/icons/logoWhite.svg"

export const Logo = (props: { className?: string; dark?: boolean }) => (
    <div
        className={classNames(
            "flex items-center translate-x-1 space-x-6",
            props.className
        )}
    >
        <img
            src={props.dark ? LogoWhite : LogoBlack}
            className="w-4 h-full w-6 transform md:-translate-y-0.5"
            alt="Logo"
        />
        <h1 className={classNames(
            props.dark ? "text-white" : "text-gray",
            "text-xl font-medium",
           "outline-none font-semibold hover:no-underline tracking-fractal"
        )}>
            FRACTAL</h1>
    </div>
)
