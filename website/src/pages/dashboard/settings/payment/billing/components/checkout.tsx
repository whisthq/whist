/* eslint-disable no-console */
import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import moment from "moment"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import history from "shared/utils/history"
import { StripeInfo, PaymentFlow } from "store/reducers/dashboard/default"

import { getZipState } from "shared/utils/stripe"
import { TAX_RATES, PLANS } from "shared/constants/stripe"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"
import { PAYMENT_IDS } from "testing/utils/testIDs"

const Checkout = (props: {
    dispatch: Dispatch<any>
    stripeInfo: StripeInfo
    paymentFlow: PaymentFlow
    editingCard: boolean
    testDate?: boolean
}) => {
    /*
        Checkout form that shows confirmation info and the submit payment button
 
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            stripeInfo (StripeInfo): StripeInfo from Redux state
            paymentFlow (PaymentFlow): PaymentFlow from Redux state
            editingCard (boolean): true/false whether the user is editing their card info
            testDate (boolean): For testing purposes so that the date stays consistent in snapshots
    */

    const { dispatch, stripeInfo, paymentFlow, editingCard } = props

    const [trialEnd, setTrialEnd] = useState("")
    const [tax, setTax] = useState(0)
    const [submittingPayment, setSubmittingPayment] = useState(false)
    const [paymentWarning, setPaymentWarning] = useState(false)

    const paymentReady =
        stripeInfo.cardBrand && stripeInfo.cardLastFour && !editingCard
    const monthlyPrice = PLANS[paymentFlow.plan]
        ? PLANS[paymentFlow.plan].price
        : 0

    useEffect(() => {
        // used for testing Date, removes nondeterminism
        const date =
            props.testDate === true || props.testDate === false
                ? new Date(2020, 9, 25)
                : new Date()
        var unix = Math.round((date.getTime() + 7 * 60000 * 60 * 24) / 1000)
        setTrialEnd(moment.unix(unix).format("MMMM Do, YYYY"))
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    useEffect(() => {
        let postalCode: string = ""
        if (stripeInfo.postalCode == null) {
            console.error("Error: postalCode is null or undefined")
        } else {
            postalCode = stripeInfo.postalCode as string
        }
        if (postalCode.length === 5) {
            const billingState = getZipState(postalCode)
            if (billingState) {
                setTax(monthlyPrice * TAX_RATES[billingState] * 0.01)
            }
        }
    }, [stripeInfo.postalCode, monthlyPrice])

    useEffect(() => {
        if (stripeInfo.stripeRequestReceived) {
            setSubmittingPayment(false)
            if (stripeInfo.checkoutStatus === "success") {
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        plan: paymentFlow.plan,
                        stripeRequestReceived: false,
                        checkoutStatus: null,
                    })
                )
                history.push("/dashboard/settings/payment/confirmation")
            } else if (stripeInfo.checkoutStatus === "failure") {
                setPaymentWarning(true)
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        stripeRequestReceived: false,
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
        <div
            className={
                paymentWarning
                    ? "bg-red-50 rounded p-10"
                    : "bg-blue-lightest rounded p-10"
            }
        >
            <div data-testid={PAYMENT_IDS.INFO}>
                <div>
                    Subtotal:{" "}
                    <div className="text-md float-right font-medium">
                        ${monthlyPrice.toFixed(2)} USD
                    </div>
                </div>
                <div>
                    Tax:{" "}
                    <div className="text-md float-right font-medium">
                        ${tax.toFixed(2)} USD
                    </div>
                </div>
                <div className="mt-8 mb-4 border border-blue-200 w-full" />
                <div>
                    Total:{" "}
                    <div className="text-md float-right font-medium">
                        ${(monthlyPrice + tax).toFixed(2)} USD
                    </div>
                </div>
                <div data-testid={PAYMENT_IDS.SUBMIT}>
                    <button
                        className="text-white hover:text-black rounded bg-blue border-none px-20 py-3 mt-8 font-medium hover:bg-mint duration-500 tracking-wide w-full"
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
                            <span>Submit</span>
                        )}
                    </button>
                </div>
                {paymentWarning ? (
                    <div data-testid={PAYMENT_IDS.WARN}>
                        <div className="mt-8 text-red-500 text-xs tracking-wide leading-normal">
                            There was an error submitting your payment. Please
                            make sure that your card info and zip code are
                            correct.
                        </div>
                    </div>
                ) : (
                    <div className="text-xs mt-8 leading-normal tracking-wide">
                        Note: You will not be charged until{" "}
                        <span className="font-medium">{trialEnd}</span>, which
                        is the end of your extended 7-day free trial. After
                        that, you will be automatically charged every month for
                        the cost of your current plan.
                    </div>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    DashboardReducer: { stripeInfo: StripeInfo; paymentFlow: PaymentFlow }
}) => {
    return {
        stripeInfo: state.DashboardReducer.stripeInfo,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Checkout)
