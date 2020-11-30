import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { FaEdit } from "react-icons/fa"
import { loadStripe } from "@stripe/stripe-js"
import {
    Elements,
    CardElement,
    useStripe,
    useElements,
} from "@stripe/react-stripe-js"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import { config } from "shared/constants/config"
import * as PureAuthAction from "store/actions/auth/pure"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"

import { cards } from "pages/profile/constants/cards"

import "styles/profile.css"

const stripePromise = loadStripe(config.keys.STRIPE_PUBLIC_KEY)
const STRIPE_OPTIONS = {
    fonts: [
        {
            cssSrc:
                "https://fonts.googleapis.com/css2?family=Maven+Pro&display=swap",
        },
    ],
}

const CardField = (props: any) => {
    const {
        dispatch,
        setEditingCard,
        savingCard,
        setSavingCard,
        warning,
        setWarning,
        setCurrentBrand,
        setCurrentLastFour,
    } = props

    const stripe = useStripe()
    const elements = useElements()

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
                        if (source.card && source.id) {
                            setCurrentBrand(source.card.brand)
                            setCurrentLastFour(source.card.last4)
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
                    color: "#777777",
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
                className={warning ? "MyCardElementWarning" : "MyCardElement"}
                options={CARD_OPTIONS}
                onChange={() => setWarning(false)}
            />
            <div
                style={{
                    display: "flex",
                    flexDirection: "row",
                    justifyContent: "space-between",
                }}
            >
                <button type="submit" className="save-button">
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
                    className="white-button"
                    style={{
                        width: "47%",
                        padding: "15px 0px",
                        fontSize: "16px",
                        marginTop: "20px",
                    }}
                    onClick={() => setEditingCard(false)}
                >
                    CANCEL
                </button>
            </div>
        </form>
    )
}

const PaymentForm = (props: {
    dispatch: any
    user: { cardBrand: string | null; cardLastFour: string | null }
    stripeInfo: any
}) => {
    const { dispatch, user, stripeInfo } = props
    const [editingCard, setEditingCard] = useState(false)
    const [savingCard, setSavingCard] = useState(false)
    const [savedCard, setSavedCard] = useState(false)
    const [warning, setWarning] = useState(false) // warning if card is invalid
    const [currentBrand, setCurrentBrand] = useState("")
    const [currentLastFour, setCurrentLastFour] = useState("")

    const cardImage =
        user.cardBrand && cards[user.cardBrand] ? cards[user.cardBrand] : null

    useEffect(() => {
        dispatch(
            PaymentPureAction.updateStripeInfo({
                stripeRequestRecieved: false,
            })
        )
    }, [dispatch])

    useEffect(() => {
        if (stripeInfo.stripeRequestRecieved && editingCard) {
            if (stripeInfo.stripeStatus === "success") {
                setEditingCard(false)
                setSavingCard(false)
                setSavedCard(true)
                dispatch(
                    PureAuthAction.updateUser({
                        cardBrand: currentBrand,
                        cardLastFour: currentLastFour,
                    })
                )
            } else if (stripeInfo.stripeStatus === "failure") {
                setWarning(true)
                setSavingCard(false)
            }
            dispatch(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: false,
                    stripeStatus: null,
                })
            )
        }
    }, [dispatch, stripeInfo, editingCard, currentBrand, currentLastFour])

    return (
        <>
            <div className="section-title">Payment Information</div>
            <div className="section-info">
                {editingCard ? (
                    <Elements stripe={stripePromise} options={STRIPE_OPTIONS}>
                        <CardField
                            dispatch={dispatch}
                            setEditingCard={setEditingCard}
                            savingCard={savingCard}
                            setSavingCard={setSavingCard}
                            warning={warning}
                            setWarning={setWarning}
                            setCurrentBrand={setCurrentBrand}
                            setCurrentLastFour={setCurrentLastFour}
                        />
                    </Elements>
                ) : (
                    <>
                        {user.cardBrand && user.cardLastFour ? (
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "row",
                                    alignItems: "center",
                                }}
                            >
                                {cardImage && (
                                    <img
                                        src={cardImage}
                                        className="card-image"
                                        alt=""
                                    />
                                )}
                                <div>**** **** **** {user.cardLastFour}</div>
                            </div>
                        ) : (
                            <div
                                className="add-name"
                                onClick={() => {
                                    setEditingCard(true)
                                    setWarning(false)
                                    setSavedCard(false)
                                }}
                            >
                                <FaEdit style={{ fontSize: "20px" }} />
                                <div
                                    style={{
                                        marginLeft: "15px",
                                        fontStyle: "italic",
                                    }}
                                >
                                    Add a card
                                </div>
                            </div>
                        )}
                        {user.cardBrand && user.cardLastFour && (
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "row",
                                }}
                            >
                                {savedCard && (
                                    <div className="saved">Saved!</div>
                                )}
                                <FaEdit
                                    className="add-name"
                                    onClick={() => {
                                        setEditingCard(true)
                                        setWarning(false)
                                        setSavedCard(false)
                                    }}
                                    style={{ fontSize: "20px" }}
                                />
                            </div>
                        )}
                    </>
                )}
            </div>
        </>
    )
}

function mapStateToProps(state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any }
}) {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(PaymentForm)
