import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { FaEdit } from "react-icons/fa"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"

import { cards } from "shared/constants/stripe"
import CardField from "shared/components/cardField"

import "styles/profile.css"

const PaymentForm = (props: {
    dispatch: any
    user: { cardBrand: string | null; cardLastFour: string | null }
    stripeInfo: any
}) => {
    const { dispatch, user, stripeInfo } = props
    const [editingCard, setEditingCard] = useState(false)
    const [savedCard, setSavedCard] = useState(false)

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
        if (
            stripeInfo.stripeRequestRecieved &&
            editingCard &&
            stripeInfo.stripeStatus === "success"
        ) {
            setSavedCard(true)
        }
    }, [stripeInfo, editingCard])

    return (
        <>
            <div className="section-title">Payment Information</div>
            <div className="section-info">
                {editingCard ? (
                    <CardField setEditingCard={setEditingCard} />
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
                                className="add"
                                onClick={() => {
                                    setEditingCard(true)
                                    setSavedCard(false)
                                }}
                            >
                                + Add a card
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
                                    className="edit"
                                    onClick={() => {
                                        setEditingCard(true)
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

const mapStateToProps = (state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(PaymentForm)
