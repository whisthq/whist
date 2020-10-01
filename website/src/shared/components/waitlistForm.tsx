import React, { useState, useContext } from "react"
import { connect } from "react-redux"
import { Button } from "react-bootstrap"
import Popup from "reactjs-popup"
import { CountryDropdown } from "react-country-region-selector"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"
import { nanoid } from "nanoid"

import history from "shared/utils/history"
import { db } from "shared/utils/firebase"
import MainContext from "shared/context/mainContext"
import {
    INITIAL_POINTS,
    REFERRAL_POINTS,
    getUnsortedLeaderboard,
} from "shared/utils/points"
import {
    insertWaitlistAction,
    deleteUserAction,
} from "store/actions/auth/waitlist"

import "styles/landing.css"

function WaitlistForm(props: any) {
    const { dispatch, user, waitlist, isAction } = props
    const { width, referralCode } = useContext(MainContext)

    const [email, setEmail] = useState("")
    const [name, setName] = useState("")
    const [country, setCountry] = useState("United States")
    const [processing, setProcessing] = useState(false)

    function updateEmail(evt: any) {
        evt.persist()
        setEmail(evt.target.value)
    }

    function updateName(evt: any) {
        evt.persist()
        setName(evt.target.value)
    }

    function updateCountry(country: string) {
        setCountry(country)
    }

    function getReferrer() {
        if (!referralCode) {
            return null
        }

        for (var i = 0; i < waitlist.length; i++) {
            if (
                waitlist[i].referralCode &&
                waitlist[i].referralCode === referralCode
            ) {
                return waitlist[i].email
            }
        }

        return null
    }

    async function insertWaitlist() {
        setProcessing(true)

        const unsortedLeaderboard = await getUnsortedLeaderboard()
        const exists = unsortedLeaderboard.hasOwnProperty(email)
        var newPoints: number = INITIAL_POINTS
        var newReferralCode = nanoid(8)

        if (!exists) {
            var referrer = getReferrer()

            if (referrer) {
                unsortedLeaderboard[referrer] = {
                    ...unsortedLeaderboard[referrer],
                    referrals: unsortedLeaderboard[referrer].referrals + 1,
                    points:
                        unsortedLeaderboard[referrer].points + REFERRAL_POINTS,
                    name: unsortedLeaderboard[referrer].name,
                    email: referrer,
                    referralCode: unsortedLeaderboard[referrer].referralCode
                        ? unsortedLeaderboard[referrer].referralCode
                        : null,
                }
                newPoints = newPoints + REFERRAL_POINTS
            }

            unsortedLeaderboard[email] = {
                referrals: 0,
                points: newPoints,
                name: name,
                email: email,
                referralCode: newReferralCode,
                country: country,
            }

            db.collection("metadata")
                .doc("waitlist")
                .update({
                    leaderboard: unsortedLeaderboard,
                })
                .then(() =>
                    dispatch(
                        insertWaitlistAction(
                            email,
                            name,
                            newPoints,
                            unsortedLeaderboard[email].referralCode,
                            0
                        )
                    )
                )
                .then(() => history.push("/application"))
        } else {
            dispatch(
                insertWaitlistAction(
                    email,
                    unsortedLeaderboard[email].name,
                    unsortedLeaderboard[email].points,
                    unsortedLeaderboard[email].referralCode,
                    0
                )
            )
        }
    }

    return (
        <div style={{ width: isAction ? "100%" : "" }}>
            {user && user.email ? (
                <div>
                    <button
                        className="white-button"
                        style={{ textTransform: "uppercase" }}
                        onClick={() => dispatch(deleteUserAction())}
                    >
                        LOGOUT AS {user.name}
                    </button>
                </div>
            ) : (
                <Popup
                    trigger={
                        isAction ? (
                            <button className="action">
                                <div
                                    style={{
                                        fontSize: width > 720 ? 20 : 16,
                                    }}
                                >
                                    Join Waitlist
                                </div>
                                <div className="points"> +100 points</div>
                            </button>
                        ) : (
                            <button className="white-button">
                                JOIN WAITLIST
                            </button>
                        )
                    }
                    modal
                    contentStyle={{
                        width: width > 720 ? 720 : "90%",
                        borderRadius: 5,
                        backgroundColor: "white",
                        border: "none",
                        padding: 0,
                        textAlign: "center",
                        boxShadow: "0px 4px 30px rgba(0,0,0,0.1)",
                        position: width > 720 ? "absolute" : "relative",
                        left: 0,
                        right: 0,
                        marginLeft: "auto",
                        marginRight: "auto",
                        top: width > 720 ? 50 : 0,
                    }}
                >
                    <div style={{ borderRadius: 5 }}>
                        <div
                            style={{
                                width: "100%",
                                display: width > 720 ? "flex" : "block",
                                justifyContent: "space-between",
                                zIndex: 100,
                                padding: 0,
                            }}
                        >
                            <input
                                type="text"
                                placeholder="Email Address"
                                onChange={updateEmail}
                                className="waitlist-form"
                            />
                            <input
                                type="text"
                                placeholder="Name"
                                onChange={updateName}
                                className="waitlist-form"
                            />
                            <CountryDropdown
                                value={country}
                                onChange={(country) => updateCountry(country)}
                            />
                        </div>
                        <div
                            style={{
                                width: "100%",
                                padding: 0,
                            }}
                        >
                            {!processing ? (
                                <Button
                                    onClick={insertWaitlist}
                                    className="waitlist-button"
                                    disabled={
                                        email && name && country ? false : true
                                    }
                                    style={{
                                        opacity:
                                            email && name && country
                                                ? 1.0
                                                : 0.8,
                                        color: "white",
                                    }}
                                >
                                    JOIN WAITLIST
                                </Button>
                            ) : (
                                <Button
                                    className="waitlist-button"
                                    disabled={true}
                                    style={{
                                        opacity:
                                            email && name && country
                                                ? 1.0
                                                : 0.8,
                                    }}
                                >
                                    <FontAwesomeIcon
                                        icon={faCircleNotch}
                                        spin
                                        style={{
                                            color: "white",
                                        }}
                                    />
                                </Button>
                            )}
                        </div>
                    </div>
                </Popup>
            )}
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any; waitlist: any } }) {
    return {
        user: state.AuthReducer.user,
        waitlist: state.AuthReducer.waitlist,
    }
}

export default connect(mapStateToProps)(WaitlistForm)
