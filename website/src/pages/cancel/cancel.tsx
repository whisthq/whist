import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import history from "shared/utils/history"

import Header from "shared/components/header"
import { PLANS } from "shared/constants/stripe"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"

import { config } from "shared/constants/config"

import "styles/profile.css"

const Cancel = (props: any) => {
    const { dispatch, user, stripeInfo } = props

    const [feedback, setFeedback] = useState("")
    const [submitting, setSubmitting] = useState(false)
    const [warning, setWarning] = useState(false)

    const valid_user = user.user_id && user.user_id !== ""

    useEffect(() => {
        if (stripeInfo.stripeRequestRecieved) {
            setSubmitting(false)
            if (stripeInfo.stripeStatus === "success") {
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        plan: null,
                        stripeRequestRecieved: false,
                        stripeStatus: null,
                    })
                )
                history.push("/canceled")
            } else if (stripeInfo.stripeStatus === "failure") {
                setWarning(true)
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        stripeRequestRecieved: false,
                        stripeStatus: null,
                    })
                )
            }
        }
    }, [dispatch, stripeInfo])

    const updateFeedback = (evt: any) => {
        setFeedback(evt.target.value)
    }

    const cancelPlan = () => {
        setSubmitting(true)
        dispatch(PaymentSideEffect.deleteSubscription(feedback))
    }

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
                    <h3 style={{ marginBottom: "30px" }}>Cancel Plan</h3>
                    <div style={{ marginBottom: "30px" }}>
                        Are you sure you want to cancel the following plan:{" "}
                        <span className="bold">
                            {stripeInfo.plan} - $
                            {PLANS[stripeInfo.plan].price.toFixed(2)} /mo (
                            {PLANS[stripeInfo.plan].subtext})
                        </span>
                        ?
                    </div>
                    <div style={{ marginBottom: "15px" }}>
                        If so, could you briefly describe why you are canceling
                        your plan, or provide any feedback that could improve
                        Fractal?
                    </div>
                    <textarea
                        value={feedback}
                        placeholder="Your feedback here!"
                        onChange={updateFeedback}
                        wrap="soft"
                        style={{
                            width: "100%",
                            border: "solid 0.8px #cccccc",
                            outline: "none",
                            padding: "20px",
                            fontSize: 14,
                            color: "#black",
                            height: "160px",
                        }}
                    />
                    {warning && (
                        <div
                            style={{
                                marginTop: 15,
                                color: "#fc3d03",
                                fontSize: 14,
                            }}
                        >
                            There was a problem canceling your plan. Please try
                            again.
                        </div>
                    )}
                    <button
                        className="save-button"
                        style={{ width: "100%" }}
                        onClick={cancelPlan}
                    >
                        {submitting ? (
                            <FontAwesomeIcon
                                icon={faCircleNotch}
                                spin
                                style={{
                                    color: "white",
                                }}
                            />
                        ) : (
                            <span>Cancel Plan</span>
                        )}
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

const mapStateToProps = (state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(Cancel)
