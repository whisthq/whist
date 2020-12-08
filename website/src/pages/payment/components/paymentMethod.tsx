import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faChevronLeft } from "@fortawesome/free-solid-svg-icons"

import history from "shared/utils/history"

import { CARDS, PLANS } from "shared/constants/stripe"
import CardField from "shared/components/cardField"

import "styles/payment.css"

const PaymentMethod = (props: {
    dispatch: any
    stripeInfo: any
    paymentFlow: { plan: string }
    editingCard: boolean
    setEditingCard: any
}) => {
    const { stripeInfo, paymentFlow, editingCard, setEditingCard } = props

    const [savedCard, setSavedCard] = useState(false)

    const hasCard = stripeInfo.cardBrand && stripeInfo.cardLastFour
    const cardImage =
        stripeInfo.cardBrand && CARDS[stripeInfo.cardBrand]
            ? CARDS[stripeInfo.cardBrand]
            : null

    useEffect(() => {
        if (
            stripeInfo.stripeRequestRecieved &&
            editingCard &&
            stripeInfo.stripeStatus === "success"
        ) {
            setSavedCard(true)
        }
    }, [stripeInfo, editingCard])

    const goBack = () => {
        history.goBack()
    }

    return (
        <div className="paymentMethod">
            <div className="back" onClick={goBack}>
                <FontAwesomeIcon
                    icon={faChevronLeft}
                    style={{
                        color: "#555555",
                        marginRight: 10,
                    }}
                />
                Choose Plan
            </div>
            <h3>Payment method</h3>
            <div>
                Your plan: {paymentFlow.plan} - $
                {PLANS[paymentFlow.plan].price.toFixed(2)} /mo (
                {PLANS[paymentFlow.plan].subtext})
            </div>
            <div style={{ marginTop: 20 }}>
                Your first seven days are free, and you can cancel anytime.
            </div>
            <div className="card-title">Your card</div>
            {hasCard ? (
                <>
                    {editingCard ? (
                        <CardField setEditingCard={setEditingCard} />
                    ) : (
                        <div
                            style={{
                                display: "flex",
                                flexDirection: "row",
                                justifyContent: "space-between",
                                alignItems: "center",
                            }}
                        >
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
                                <div>
                                    **** **** **** {stripeInfo.cardLastFour}
                                </div>
                            </div>
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "row",
                                }}
                            >
                                {savedCard && (
                                    <div className="saved">Saved!</div>
                                )}
                                <div
                                    className="update"
                                    onClick={() => {
                                        setEditingCard(true)
                                        setSavedCard(false)
                                    }}
                                >
                                    Update
                                </div>
                            </div>
                        </div>
                    )}
                </>
            ) : (
                <>
                    {editingCard ? (
                        <CardField setEditingCard={setEditingCard} />
                    ) : (
                        <div
                            className="update"
                            onClick={() => {
                                setEditingCard(true)
                                setSavedCard(false)
                            }}
                        >
                            + Add a card
                        </div>
                    )}
                </>
            )}
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

export default connect(mapStateToProps)(PaymentMethod)
