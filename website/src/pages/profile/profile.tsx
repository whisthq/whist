import React, { useEffect } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import NameForm from "pages/profile/components/nameForm"
import PasswordForm from "pages/profile/components/passwordForm"
import PaymentForm from "pages/profile/components/paymentForm"
import PlanForm from "pages/profile/components/planForm"
import * as PureAuthAction from "store/actions/auth/pure"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT as AUTH_DEFAULT } from "store/reducers/auth/default"
import { DEFAULT as DASHBOARD_DEFAULT } from "store/reducers/dashboard/default"

import { config } from "shared/constants/config"

import "styles/profile.css"

const Profile = (props: any) => {
    const { user, dispatch } = props

    const validUser = user.userID && user.userID !== ""

    const handleSignOut = () => {
        dispatch(PureAuthAction.updateUser(deepCopy(AUTH_DEFAULT.user)))
        dispatch(PureAuthAction.updateAuthFlow(deepCopy(AUTH_DEFAULT.authFlow)))
        dispatch(
            PaymentPureAction.updateStripeInfo(
                deepCopy(DASHBOARD_DEFAULT.stripeInfo)
            )
        )
        dispatch(
            PaymentPureAction.updatePaymentFlow(
                deepCopy(DASHBOARD_DEFAULT.paymentFlow)
            )
        )
    }

    // Clear redux values on page load that will be set when editing password
    useEffect(() => {
        dispatch(
            PureAuthAction.updateAuthFlow({
                resetDone: false,
                passwordVerified: null,
            })
        )
    }, [dispatch])

    if (!validUser) {
        return <Redirect to="/auth/bypass" />
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
                    <h2 style={{ marginBottom: "20px" }}>Your Profile</h2>
                    <div className="section-title">Email</div>
                    <div className="section-info">{user.userID}</div>
                    <NameForm />
                    <PasswordForm />
                    {config.payment_enabled && (
                        <>
                            <PlanForm />
                            <PaymentForm />
                        </>
                    )}
                    <button
                        className="white-button"
                        style={{
                            width: "100%",
                            marginTop: 25,
                            fontSize: 16,
                        }}
                        onClick={handleSignOut}
                    >
                        Sign Out
                    </button>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Profile)
