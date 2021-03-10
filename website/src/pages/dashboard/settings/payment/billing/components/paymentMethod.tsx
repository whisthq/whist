import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"

import { CARDS } from "shared/constants/stripe"
import CardField from "shared/components/cardField"
// import { PriceBox } from "pages/dashboard/settings/payment/plan/components/priceBox"

import { PaymentFlow, StripeInfo } from "shared/types/reducers"

const PaymentMethod = (props: {
    dispatch: Dispatch<any>
    stripeInfo: StripeInfo
    paymentFlow: PaymentFlow
    editingCard: boolean
    setEditingCard: any
}) => {
    /*
        Form that lets the user add or change their current payment method
 
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            stripeInfo (StripeInfo): StripeInfo from Redux state
            paymentFlow (PaymentFlow): PaymentFlow from Redux state
            editingCard (boolean): true/false whether the user is editing their card info
            setEditingCard (any): Callback function to set whether card is being edited
    */

    const { stripeInfo, paymentFlow, editingCard, setEditingCard } = props

    const [savedCard, setSavedCard] = useState(false)

    const hasCard = stripeInfo.cardBrand && stripeInfo.cardLastFour
    const cardImage =
        stripeInfo.cardBrand && CARDS[stripeInfo.cardBrand]
            ? CARDS[stripeInfo.cardBrand]
            : null

    useEffect(() => {
        if (
            stripeInfo.stripeRequestReceived &&
            editingCard &&
            stripeInfo.stripeStatus === "success"
        ) {
            setSavedCard(true)
        }
    }, [stripeInfo, editingCard])

    return (
        <div className="relative pr-4">
            <div className="mt-4 text-4xl font-medium tracking-wide">
                Enter a Payment Method
            </div>
            <div className="font-body mt-6">
                You have selected the{" "}
                <span className="font-medium tracking-wide">
                    {paymentFlow.plan} plan
                </span>
                . Your first seven days are free, and you can cancel anytime.
            </div>
            <div className="font-body font-semibold mt-8 mb-4 font-medium">Your card</div>
            {hasCard ? (
                <>
                    {editingCard ? (
                        <CardField setEditingCard={setEditingCard} />
                    ) : (
                        <div>
                            <div className="flex items-center">
                                {cardImage && (
                                    <img
                                        src={cardImage}
                                        className="h-4 mr-6"
                                        alt=""
                                    />
                                )}
                                    <div className="font-body">
                                    **** **** **** {stripeInfo.cardLastFour}
                                </div>
                            </div>
                            <div className="flex justify-between">
                                <div
                                    className="font-body bg-blue-lightest text-black px-4 py-1 mt-3 text-xs rounded font-medium cursor-pointer"
                                    onClick={() => {
                                        setEditingCard(true)
                                        setSavedCard(false)
                                    }}
                                >
                                    Update
                                </div>
                                {savedCard && (
                                    <div className="mt-3 text-sm text-green">
                                        Saved!
                                    </div>
                                )}
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
                            className="cursor-pointer"
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
    DashboardReducer: { stripeInfo: StripeInfo; paymentFlow: PaymentFlow }
}) => {
    return {
        stripeInfo: state.DashboardReducer.stripeInfo,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(PaymentMethod)
