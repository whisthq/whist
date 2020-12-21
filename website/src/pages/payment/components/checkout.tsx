import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import moment from "moment"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import history from "shared/utils/history"

import { getZipState } from "shared/utils/stripe"
import { TAX_RATES, PLANS } from "shared/constants/stripe"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"

import "styles/payment.css"

const Checkout = (props: {
    dispatch: any
    stripeInfo: any
    paymentFlow: { plan: string }
    editingCard: boolean
}) => {
    const { dispatch, stripeInfo, paymentFlow, editingCard } = props

    const [trialEnd, setTrialEnd] = useState("")
    const [tax, setTax] = useState(0)
    const [submittingPayment, setSubmittingPayment] = useState(false)
    const [paymentWarning, setPaymentWarning] = useState(false)

    const paymentReady =
        stripeInfo.cardBrand && stripeInfo.cardLastFour && !editingCard
    const monthlyPrice = PLANS[paymentFlow.plan].price

    useEffect(() => {
        var unix = Math.round(
            (new Date().getTime() + 7 * 60000 * 60 * 24) / 1000
        )
        setTrialEnd(moment.unix(unix).format("MMMM Do, YYYY"))
    }, [])

    useEffect(() => {
        if (stripeInfo.postalCode.length === 5) {
            const billingState = getZipState(stripeInfo.postalCode)
            if (billingState) {
                setTax(monthlyPrice * TAX_RATES[billingState] * 0.01)
            }
        }
    }, [stripeInfo.postalCode, monthlyPrice])

    useEffect(() => {
        if (stripeInfo.stripeRequestRecieved) {
            setSubmittingPayment(false)
            if (stripeInfo.checkoutStatus === "success") {
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        plan: paymentFlow.plan,
                        stripeRequestRecieved: false,
                        checkoutStatus: null,
                    })
                )
                history.push("/confirmation")
            } else if (stripeInfo.checkoutStatus === "failure") {
                setPaymentWarning(true)
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        stripeRequestRecieved: false,
                        checkoutStatus: null,
                    })
                )
            }
        }
    }, [dispatch, stripeInfo, paymentFlow.plan])

    const submitPayment = () => {
        dispatch(PaymentSideEffect.addSubscription(paymentFlow.plan))
        setPaymentWarning(false)
        setSubmittingPayment(true)
    }

    return (
        <div className={paymentWarning ? "checkout-warning" : "checkout"}>
            <div>
                Subtotal:{" "}
                <div className="price">${monthlyPrice.toFixed(2)} USD</div>
            </div>
            <div>
                Tax: <div className="price">${tax.toFixed(2)} USD</div>
            </div>
            <div className="line" />
            <div>
                Total:{" "}
                <div className="price">
                    ${(monthlyPrice + tax).toFixed(2)} USD
                </div>
            </div>
            <button
                className="pay-button"
                style={{ opacity: paymentReady ? "1" : ".6" }}
                disabled={!paymentReady}
                onClick={submitPayment}
            >
                {submittingPayment ? (
                    <FontAwesomeIcon
                        icon={faCircleNotch}
                        spin
                        style={{
                            color: "white",
                        }}
                    />
                ) : (
                    <span>Submit Payment</span>
                )}
            </button>
            {paymentWarning && (
                <div style={{ color: "#fc3d03", fontSize: 12 }}>
                    There was an error submitting your payment. Please try
                    again.
                </div>
            )}
            <div style={{ marginTop: 10, fontSize: 12 }}>
                Note: You will not be charged until{" "}
                <span className="bold">{trialEnd}</span>, which is the end of
                your 7-day free trial. After that, you will be automatically
                charged every month for the cost of your current plan. You can
                cancel your plan at anytime.
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    DashboardReducer: { stripeInfo: any; paymentFlow: any }
}) => {
    return {
        stripeInfo: state.DashboardReducer.stripeInfo,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Checkout)
