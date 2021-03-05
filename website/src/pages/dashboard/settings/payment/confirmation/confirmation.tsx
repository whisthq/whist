import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import moment from "moment"
import { Redirect } from "react-router"

import history from "shared/utils/history"
import { config } from "shared/constants/config"
import { StripeInfo } from "store/reducers/dashboard/default"
import { User } from "store/reducers/auth/default"
import { CONFIRMATION_IDS } from "testing/utils/testIDs"

const Confirmation = (props: { user: User; stripeInfo: StripeInfo }) => {
    /*
        After users have paid for a plan, show them this page
 
        Arguments:
            user (User): User from Redux state
            stripeInfo (StripeInfo): StripeInfo from Redux state
    */

    const { user, stripeInfo } = props

    const validUser = user.userID && user.userID !== ""

    const [trialEnd, setTrialEnd] = useState("")

    useEffect(() => {
        var unix = Math.round(
            (new Date().getTime() + 7 * 60000 * 60 * 24) / 1000
        )
        setTrialEnd(moment.unix(unix).format("MMMM Do, YYYY"))
    }, [])

    const backToProfile = () => {
        history.push("/dashboard/settings/payment")
    }

    if (!validUser) {
        return <Redirect to="/auth" />
    } else if (!stripeInfo.plan || !config.payment_enabled) {
        return <Redirect to="/dashboard/settings" />
    } else {
        return (
            <div className="relative m-auto pt-8 max-w-screen-sm">
                <div data-testid={CONFIRMATION_IDS.INFO}>
                    <div className="mt-4 text-4xl font-medium tracking-wide">
                        Congratulations!
                    </div>
                    <div className="mt-6">
                        You've successfully upgraded your plan and won't be
                        charged until {trialEnd}.
                    </div>
                </div>
                <div data-testid={CONFIRMATION_IDS.BACK}>
                    <button
                        className="text-left text-white font-medium mt-8 bg-blue px-12 py-2.5 rounded hover:bg-mint hover:text-black"
                        onClick={backToProfile}
                        style={{ transitionDuration: "0.5s" }}
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
