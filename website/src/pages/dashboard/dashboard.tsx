import React from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import { PuffAnimation } from "shared/components/loadingAnimations"

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
                        width: 400,
                        margin: "auto",
                        marginTop: 70,
                    }}
                >
                    <div
                        style={{
                            color: "#111111",
                            textAlign: "center",
                            fontSize: 32,
                        }}
                    >
                        Welcome back {user.user_id}.
                    </div>
                    <div
                        style={{
                            color: "#3930b8",
                            textAlign: "center",
                        }}
                    >
                        We will be setting up your account soon. Thank you for
                        registering with us.
                    </div>
                </div>
                <div>
                    <PuffAnimation />
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
