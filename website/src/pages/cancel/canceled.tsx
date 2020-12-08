import React from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import history from "shared/utils/history"

import Header from "shared/components/header"

import { config } from "shared/constants/config"

const Canceled = (props: any) => {
    const { user } = props

    const valid_user = user.user_id && user.user_id !== ""

    const addPlan = () => {
        history.push("/plan")
    }

    const backToProfile = () => {
        history.push("/profile")
    }

    if (!valid_user) {
        return <Redirect to="/auth/bypass" />
    } else if (!config.payment_enabled) {
        return <Redirect to="/profile" />
    } else {
        return (
            <div className="fractalContainer">
                <Header dark={false} account />
                <div
                    style={{
                        width: 500,
                        margin: "auto",
                        marginTop: 70,
                    }}
                >
                    <h3 style={{ marginBottom: "30px" }}>
                        Successfully canceled plan.
                    </h3>
                    <button
                        className="save-button"
                        style={{ width: "100%" }}
                        onClick={addPlan}
                    >
                        Add a New Plan
                    </button>
                    <button
                        className="white-button"
                        style={{ width: "100%", fontSize: 16, marginTop: 20 }}
                        onClick={backToProfile}
                    >
                        Back to Profile
                    </button>
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: { AuthReducer: { user: any } }) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Canceled)
