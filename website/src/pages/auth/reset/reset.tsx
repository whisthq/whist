import React, { Dispatch } from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router-dom"

import { updateAuthFlow } from "store/actions/auth/pure"

import Header from "shared/components/header"
import ResetView from "pages/auth/reset/components/resetView"
import { HEADER } from "testing/utils/testIDs"

const Reset = (props: { dispatch: Dispatch<any>; testSearch: string }) => {
    const { dispatch } = props
    // other thing is for testing
    const useLocationSearch = useLocation().search
    const search = props.testSearch ? props.testSearch : useLocationSearch
    const token = search.substring(1, search.length)
    const validToken = token && token.length >= 1

    if (!validToken) {
        return <Redirect to="/auth" />
    } else {
        return (
            <>
                <ResetView token={token} validToken={validToken} />
            </>
        )
    }
}

export default connect()(Reset)
