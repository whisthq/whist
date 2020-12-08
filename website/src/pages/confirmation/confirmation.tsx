import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import moment from "moment"
import { Redirect } from "react-router"

import history from "shared/utils/history"

import Header from "shared/components/header"
import { PLANS } from "shared/constants/stripe"

import { config } from "shared/constants/config"

const Confirmation = (props: { user: any; stripeInfo: any }) => {
    const { user, stripeInfo } = props

    const valid_user = user.user_id && user.user_id !== ""

    const [trialEnd, setTrialEnd] = useState("")

    useEffect(() => {
        var unix = Math.round(
            (new Date().getTime() + 7 * 60000 * 60 * 24) / 1000
        )
        setTrialEnd(moment.unix(unix).format("MMMM Do, YYYY"))
    }, [])

    const backToProfile = () => {
        history.push("/profile")
    }

    if (!valid_user) {
        return <Redirect to="/auth/bypass" />
    } else if (!stripeInfo.plan || !config.payment_enabled) {
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
                        Success! You've started your Fractal plan.
                    </h3>
                    <div>
                        Your plan is:{" "}
                        <span className="bold">
                            {stripeInfo.plan} - $
                            {PLANS[stripeInfo.plan].price.toFixed(2)} /mo (
                            {PLANS[stripeInfo.plan].subtext})
                        </span>
                        , and your next billing date is on{" "}
                        <span className="bold">{trialEnd}</span>. You can cancel
                        your plan at any time.
                    </div>
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

const mapStateToProps = (state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(Confirmation)
