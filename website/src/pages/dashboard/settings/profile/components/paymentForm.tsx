import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import { FaPencilAlt } from "react-icons/fa"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"

import { CARDS } from "shared/constants/stripe"
import CardField from "shared/components/cardField"
import { StripeInfo } from "shared/types/reducers"
import classNames from "classnames"

import {
    PROFILE_IDS,
    PAYMENT_IDS,
    E2E_DASHBOARD_IDS,
} from "testing/utils/testIDs"

import styles from "styles/profile.module.css"

const PaymentForm = (props: {
    dispatch: Dispatch<any>
    stripeInfo: StripeInfo
}) => {
    const { dispatch, stripeInfo } = props
    const [editingCard, setEditingCard] = useState(false)
    const [savedCard, setSavedCard] = useState(false)

    const cardImage =
        stripeInfo.cardBrand && CARDS[stripeInfo.cardBrand]
            ? CARDS[stripeInfo.cardBrand]
            : null

    useEffect(() => {
        dispatch(
            PaymentPureAction.updateStripeInfo({
                stripeRequestReceived: false,
            })
        )
    }, [dispatch])

    useEffect(() => {
        if (
            stripeInfo.stripeRequestReceived &&
            editingCard &&
            stripeInfo.stripeStatus === "success"
        ) {
            setSavedCard(true)
        }
    }, [stripeInfo, editingCard])

    /** TODO: Add a "delete" icon next to the editing icon that opens a popup for users
     *   to delete their credit card (https://github.com/fractal/website/issues/333)
     */
    return (
        <div data-testid={PROFILE_IDS.PAY}>
            <div className={classNames("font-body", styles.sectionTitle)}>Payment Information</div>
            <div className={styles.sectionInfo}>
                {editingCard ? (
                    <div
                        data-testid={PAYMENT_IDS.CARDFIELD}
                        className={styles.dashedBox}
                        style={{ padding: "15px 20px" }}
                    >
                        <CardField setEditingCard={setEditingCard} />
                    </div>
                ) : (
                    <div
                        className={styles.dashedBox}
                        style={{
                            display: "flex",
                            justifyContent: "space-between",
                        }}
                    >
                        {stripeInfo.cardBrand && stripeInfo.cardLastFour ? (
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
                                        className="h-4 mr-6"
                                        alt=""
                                    />
                                )}
                                <div className="font-body">
                                    **** **** **** {stripeInfo.cardLastFour}
                                </div>
                            </div>
                        ) : (
                            <div
                                className={classNames("font-body", styles.dashedBox)}
                                data-testid={PAYMENT_IDS.ADDCARD}
                                onClick={() => {
                                    setEditingCard(true)
                                    setSavedCard(false)
                                }}
                            >
                                + Add a card
                            </div>
                        )}
                        {stripeInfo.cardBrand && stripeInfo.cardLastFour && (
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "row",
                                }}
                            >
                                {savedCard && (
                                    <div className={classNames("font-body", styles.saved)}>Saved!</div>
                                )}
                                <FaPencilAlt
                                    id={E2E_DASHBOARD_IDS.EDITCARD}
                                    className="relative top-1 cursor-pointer"
                                    onClick={() => {
                                        setEditingCard(true)
                                        setSavedCard(false)
                                    }}
                                    style={{ position: "relative", top: 5 }}
                                />
                            </div>
                        )}
                    </div>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    DashboardReducer: { stripeInfo: StripeInfo }
}) => {
    return {
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(PaymentForm)
