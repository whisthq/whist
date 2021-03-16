import React from "react"
import classNames from "classnames"
import LogoPurple from "@app/assets/icons/logoPurple.svg"

import "@app/styles/global.module.css"
import "@app/styles/tailwind.css"

export const Logo = (props: { className?: string }) => (
    <div
        className={classNames(
            "flex items-center translate-x-1 space-x-6",
            props.className
        )}
    >
        <img
            src={LogoPurple}
            className="w-12 h-12 transform md:-translate-y-0.5"
            alt="Logo"
        />
    </div>
)
