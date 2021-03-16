import React from "react"

export const AuthWarning = (props: { warning: string | null | undefined }) => {
    const { warning } = props

    if (!warning || warning === "") {
        return <></>
    } else {
        return (
            <div className="font-body w-full bg-red-100 text-red px-6 py-3 mt-4 text-sm rounded relative top-1">
                {warning}
            </div>
        )
    }
}
