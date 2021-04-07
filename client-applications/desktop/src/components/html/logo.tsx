import React from "react"
import classNames from "classnames"
import LogoPurple from "@app/renderer/assets/logoPurple.svg"

export const Logo = (props: { className?: string }) => (
    <div className={classNames("text-center", props.className)}>
        <img src={LogoPurple} className="w-12 h-12 m-auto" alt="Logo" />
    </div>
)
