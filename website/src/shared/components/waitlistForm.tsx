import React, { useState, useContext, useEffect } from "react"
import { connect } from "react-redux"
import { Button } from "react-bootstrap"
import Popup from "reactjs-popup"
import { CountryDropdown } from "react-country-region-selector"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"
import { nanoid } from "nanoid"
import { useMutation } from "@apollo/client"

import { INSERT_WAITLIST } from "pages/landing/constants/graphql"

import { UPDATE_WAITLIST_REFERRALS } from "shared/constants/graphql"
import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"
import { db } from "shared/utils/firebase"
import {
    INITIAL_POINTS,
    REFERRAL_POINTS,
    SECRET_POINTS,
} from "shared/utils/points"
import { deep_copy, deepCopy } from "shared/utils/reducerHelpers"

import * as PureAuthAction from "store/actions/auth/pure"
import * as PureWaitlistAction from "store/actions/waitlist/pure"
import { DEFAULT } from "store/reducers/auth/default"

import "styles/landing.css"
import { checkEmail } from "pages/auth/constants/authHelpers"

const WaitlistForm = (props: any) => {
    const { dispatch, waitlist, waitlistUser, user, dark, isAction } = props
    const { width, referralCode } = useContext(MainContext)

    const [email, setEmail] = useState("")
    const [name, setName] = useState("")
    const [country, setCountry] = useState("United States")
    const [processing, setProcessing] = useState(false)
    const [addWaitlistCompleted, setAddWaitlistCompleted] = useState(false)
    const [addWaitlistError, setAddWaitlistError] = useState(false)
    const [updatedPoints, setUpdatedPoints] = useState(0)
    const [updatedReferralCode, setUpdatedReferralCode] = useState("")

    const validInputs =
        email && checkEmail(email) && name && name.length > 1 && country

    const [addWaitlist] = useMutation(INSERT_WAITLIST, {
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`,
            },
        },
        onCompleted: () => {
            setAddWaitlistCompleted(true)
        },
        onError: () => {
            setAddWaitlistError(true)
        },
    })

    const [updatePoints, updatePointsResult] = useMutation(
        UPDATE_WAITLIST_REFERRALS,
        {
            context: {
                headers: {
                    Authorization: `Bearer ${user.accessToken}`,
                },
            },
        }
    )

    useEffect(() => {
        // if successfully updated database, update redux and firebase
        if (addWaitlistCompleted && !addWaitlistError) {
            const secretPoints = deep_copy(SECRET_POINTS)
            dispatch(
                PureWaitlistAction.updateWaitlistUser({
                    points: updatedPoints,
                    referralCode: updatedReferralCode,
                    userID: email,
                    name: name,
                    eastereggsAvailable: secretPoints,
                })
            )

            dispatch(
                PureWaitlistAction.updateNavigation({
                    applicationRedirect: true,
                })
            )

            db.collection("eastereggs").doc(email).set({
                available: secretPoints,
            })

            setProcessing(false)
        } else if (addWaitlistError) {
            setProcessing(false)
        }
    }, [
        addWaitlistCompleted,
        addWaitlistError,
        dispatch,
        email,
        name,
        updatedPoints,
        updatedReferralCode,
    ])

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
                return waitlist[i]
            }
        }

        return null
    }

    function getWaitlistUser() {
        if (!email) {
            return null
        }

        for (var i = 0; i < waitlist.length; i++) {
            if (waitlist[i].user_id && waitlist[i].user_id === email) {
                return waitlist[i]
            }
        }

        return null
    }

    function logout() {
        dispatch(PureWaitlistAction.resetWaitlistUser())
        dispatch(PureAuthAction.updateUser(deepCopy(DEFAULT.user)))
        dispatch(PureAuthAction.updateAuthFlow(deepCopy(DEFAULT.authFlow)))
    }

    async function insertWaitlist() {
        setAddWaitlistCompleted(false)
        setAddWaitlistError(false)
        setProcessing(true)

        const currentUser = getWaitlistUser()

        var newPoints: number = INITIAL_POINTS
        var newReferralCode = nanoid(8)

        if (!currentUser) {
            const referrer = getReferrer()

            if (referrer && referrer.userID) {
                updatePoints({
                    variables: {
                        userID: referrer.userID,
                        points: referrer.points + REFERRAL_POINTS,
                        referrals: referrer.referrals + 1,
                    },
                    optimisticResponse: true,
                })
                newPoints = newPoints + REFERRAL_POINTS
            }
            setUpdatedPoints(newPoints)
            setUpdatedReferralCode(newReferralCode)

            addWaitlist({
                variables: {
                    userID: email,
                    name: name,
                    points: newPoints,
                    referralCode: newReferralCode,
                    referrals: 0,
                    country: country,
                },
            })
        } else {
            // get the easter eggs state so we can set local if you re-log in
            const eastereggsDocument = await db
                .collection("eastereggs")
                .doc(email)
                .get()

            let data = eastereggsDocument.data()

            // if they did existed but were here before the update
            if (!eastereggsDocument.exists) {
                data = {
                    available: deepCopy(SECRET_POINTS),
                }

                db.collection("eastereggs").doc(email).set(data)
            }

            const secretPoints = data ? data.available : {}

            dispatch(
                PureWaitlistAction.updateWaitlistUser({
                    points: currentUser.points,
                    referralCode: currentUser.referralCode,
                    userID: email,
                    name: currentUser.name,
                    eastereggsAvailable: secretPoints,
                })
            )

            setProcessing(false)
        }
    }

    return (
        <div style={{ width: isAction ? "100%" : "" }}>
            {waitlistUser && waitlistUser.userID ? (
                <div>
                    <button
                        className={dark ? "white-button-light" : "white-button"}
                        style={{ textTransform: "uppercase" }}
                        onClick={logout}
                    >
                        LOGOUT FROM WAITLIST
                    </button>
                </div>
            ) : (
                <Popup
                    trigger={
                        isAction ? (
                            <button
                                className="action"
                                style={{
                                    display: "flex",
                                    justifyContent: "space-between",
                                }}
                            >
                                <div
                                    style={{
                                        fontSize:
                                            width > ScreenSize.MEDIUM ? 20 : 16,
                                    }}
                                >
                                    Join Waitlist
                                </div>
                                <div className="points">
                                    +{INITIAL_POINTS} points
                                </div>
                            </button>
                        ) : (
                            <button
                                className={
                                    dark ? "white-button-light" : "white-button"
                                }
                            >
                                JOIN WAITLIST
                            </button>
                        )
                    }
                    modal
                    contentStyle={{
                        width: width > ScreenSize.MEDIUM ? 720 : "90%",
                        borderRadius: 5,
                        backgroundColor: "white",
                        border: "none",
                        padding: 0,
                        textAlign: "center",
                        boxShadow: "0px 4px 30px rgba(0,0,0,0.1)",
                        position:
                            width > ScreenSize.MEDIUM ? "absolute" : "relative",
                        left: 0,
                        right: 0,
                        marginLeft: "auto",
                        marginRight: "auto",
                        top: width > ScreenSize.MEDIUM ? 50 : 0,
                    }}
                >
                    <div style={{ borderRadius: 5 }}>
                        <div
                            style={{
                                width: "100%",
                                display:
                                    width > ScreenSize.MEDIUM
                                        ? "flex"
                                        : "block",
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
                                    disabled={!validInputs}
                                    style={{
                                        opacity: validInputs ? 1.0 : 0.4, // not obvious w/ 0.8
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
                                        opacity: validInputs ? 1.0 : 0.4,
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
                        {updatePointsResult.error && (
                            <div className="waitlist-form-error">
                                There was an error adding referral points to
                                your friend's account. For help, please contact{" "}
                                <a
                                    href="mailto: support@tryfractal.com"
                                    className="support-link"
                                >
                                    support@tryfractal.com
                                </a>
                                .
                            </div>
                        )}
                        {addWaitlistError && (
                            <div className="waitlist-form-error">
                                There was an error adding you to the waitlist.
                                Please try again.
                            </div>
                        )}
                    </div>
                </Popup>
            )}
        </div>
    )
}

function mapStateToProps(state: {
    AuthReducer: {
        user: any
    }
    WaitlistReducer: {
        waitlistUser: any
        waitlist: any[]
        waitlistData: any
    }
}) {
    return {
        waitlist: state.WaitlistReducer.waitlistData.waitlist,
        waitlistUser: state.WaitlistReducer.waitlistUser,
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(WaitlistForm)
