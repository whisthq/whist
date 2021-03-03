import React from "react"

import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"

const AuthWarning = (props: {
    warning: string | null | undefined,
}) => {
    const {warning} = props

    if(!warning || warning === "") {
        return (
            <></>
        )
    } else {
        return(
            <div className="w-full bg-red-100 text-red px-6 py-3 mt-4 text-sm rounded relative top-1" data-testid={AUTH_IDS.WARN}>
                {warning}
            </div>
        )
    }
}

export default AuthWarning