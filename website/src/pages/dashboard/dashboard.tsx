import React from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import { PagePuff } from "shared/components/loadingAnimations"

import "styles/auth.css"

const Dashboard = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const { user } = props

    const valid_user = user.user_id && user.user_id !== ""

    if (!valid_user) {
        return <Redirect to="/" />
    } else {
        // for now it wil lalways be loading
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <div
                    style={{
                        width: "100vw",
                        height: "100vh",
                        position: "relative",
                    }}
                >
                    <PagePuff />
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Dashboard)
