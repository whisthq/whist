import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import moment from "moment"

import { getZipState } from "shared/utils/stripe"
import { TAX_RATES } from "shared/constants/stripe"

import "styles/payment.css"

const Checkout = (props: {
    dispatch: any
    user: any
    paymentFlow: { plan: string }
    editingCard: boolean
}) => {
    const { user, paymentFlow, editingCard } = props

    const [trialEnd, setTrialEnd] = useState("")
    const [tax, setTax] = useState(0)

    const prices: { [key: string]: number } = {
        Hourly: 5,
        Monthly: 39,
        Unlimited: 99,
    }

    const paymentReady = user.cardBrand && user.cardLastFour && !editingCard
    const monthlyPrice = prices[paymentFlow.plan]

    useEffect(() => {
        var unix = Math.round(
            (new Date().getTime() + 7 * 60000 * 60 * 24) / 1000
        )
        setTrialEnd(moment.unix(unix).format("MMMM Do, YYYY"))
    }, [])

    useEffect(() => {
        if (user.postalCode.length === 5) {
            const billingState = getZipState(user.postalCode)
            if (billingState) {
                setTax(monthlyPrice * TAX_RATES[billingState] * 0.01)
            }
        }
    }, [user.postalCode, monthlyPrice])

    const submitPayment = () => {}

    return (
        <div className="checkout">
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
                Submit Payment
            </button>
            <div style={{ marginTop: 20, fontSize: 12 }}>
                Note: You will not be charged until{" "}
                <span className="bold">{trialEnd}</span>, which is the end of
                your 7-day free trial. After that, you will be automatically
                charged every month for the cost of your current plan. You can
                cancel your plan at anytime.
            </div>
        </div>
    )
}

function mapStateToProps(state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any; paymentFlow: any }
}) {
    return {
        user: state.AuthReducer.user,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Checkout)
