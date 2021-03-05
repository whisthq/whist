import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import { CardElement, useStripe, useElements } from "@stripe/react-stripe-js"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"

import { StripeInfo } from "store/reducers/dashboard/default"
import { User } from "store/reducers/auth/default"
import { PAYMENT_IDS } from "testing/utils/testIDs"

const CardField = (props: {
    dispatch: Dispatch<any>
    stripeInfo: StripeInfo
    setEditingCard: any
}) => {
    const { dispatch, stripeInfo, setEditingCard } = props

    const [savingCard, setSavingCard] = useState(false)
    const [warning, setWarning] = useState(false)
    const [currentBrand, setCurrentBrand] = useState("")
    const [currentLastFour, setCurrentLastFour] = useState("")
    const [currentPostalCode, setCurrentPostalCode] = useState("")

    const stripe = useStripe()
    const elements = useElements()

    useEffect(() => {
        if (stripeInfo.stripeRequestReceived) {
            if (stripeInfo.stripeStatus === "success") {
                setSavingCard(false)
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        cardBrand: currentBrand,
                        cardLastFour: currentLastFour,
                        postalCode: currentPostalCode,
                        stripeRequestReceived: false,
                        stripeStatus: null,
                    })
                )
                setEditingCard(false)
            } else if (stripeInfo.stripeStatus === "failure") {
                setWarning(true)
                setSavingCard(false)
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        stripeRequestReceived: false,
                        stripeStatus: null,
                    })
                )
            }
        }
    }, [
        dispatch,
        stripeInfo,
        currentBrand,
        currentLastFour,
        currentPostalCode,
        setEditingCard,
    ])

    const handleSubmit = async (event: any) => {
        // Block native form submission.
        event.preventDefault()

        if (!stripe || !elements) {
            // Stripe.js has not loaded yet. Make sure to disable
            // form submission until Stripe.js has loaded.
            return
        }
        setSavingCard(true)
        setWarning(false)

        // Get a reference to a mounted CardElement. Elements knows how
        // to find your CardElement because there can only ever be one of
        // each type of element.
        const cardElement = elements.getElement(CardElement)

        if (cardElement) {
            stripe
                .createSource(cardElement, { type: "card" })
                .then((result) => {
                    if (result.error) {
                        setSavingCard(false)
                        setWarning(true)
                    } else if (result.source) {
                        const source = result.source
                        if (
                            source.card &&
                            source.card.brand &&
                            source.card.last4 &&
                            source.id
                        ) {
                            setCurrentBrand(source.card.brand)
                            setCurrentLastFour(source.card.last4)
                            if (
                                source.owner &&
                                source.owner.address &&
                                source.owner.address.postal_code
                            ) {
                                setCurrentPostalCode(
                                    source.owner.address.postal_code
                                )
                            }
                            dispatch(PaymentSideEffect.addCard(source))
                        }
                    }
                })
        }
    }

    const CARD_OPTIONS = {
        style: {
            base: {
                fontFamily: "Maven Pro",
                fontSmoothing: "antialiased",
                fontSize: "15px",
                "::placeholder": {
                    color: "#999999",
                },
            },
            invalid: {
                color: "#fa755a",
                iconColor: "#fa755a",
            },
        },
    }

    return (
        <form
            onSubmit={handleSubmit}
            style={{ width: "100%", position: "relative" }}
        >
            {warning && (
                <div
                    style={{
                        color: "white",
                        background: "#fc3d03",
                        fontSize: 12,
                        padding: "3px 25px",
                        position: "absolute",
                        right: 0,
                        top: "-35px",
                    }}
                >
                    Invalid card
                </div>
            )}
            <CardElement
                className="bg-blue-100 py-3 px-5 w-full rounded"
                options={CARD_OPTIONS}
                onChange={() => setWarning(false)}
                id={PAYMENT_IDS.CARD_FORM}
            />
            <div className="flex">
                <button
                    type="submit"
                    className="w-3/6 py-2.5 bg-blue text-white mt-4 rounded mr-2 hover:bg-mint hover:text-black duration-500"
                >
                    {savingCard ? (
                        <FontAwesomeIcon
                            icon={faCircleNotch}
                            spin
                            style={{
                                color: "white",
                            }}
                        />
                    ) : (
                        <span>SAVE</span>
                    )}
                </button>
                <button
                    className="w-5/12 py-2.5 text-gray mt-4"
                    onClick={() => setEditingCard(false)}
                >
                    CANCEL
                </button>
            </div>
        </form>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: User }
    DashboardReducer: { stripeInfo: StripeInfo }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(CardField)
