import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Button } from "react-bootstrap"
import { CountryDropdown } from "react-country-region-selector"

import history from "utils/history"
import { db } from "utils/firebase"
import { INITIAL_POINTS } from "utils/points"
import { insertWaitlistAction } from "store/actions/auth/waitlist"

import "styles/landing.css"

function WaitlistForm(props: any) {
    const { dispatch, user } = props

    const [email, setEmail] = useState("")
    const [name, setName] = useState("")
    const [country, setCountry] = useState("United States")
    const [processing, setProcessing] = useState(false)
    const [, setReferralCode] = useState()

    useEffect(() => {
        console.log("Use Effect waitlist")
    }, [])

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

    function updateReferralCode(evt: any) {
        evt.persist()
        setReferralCode(evt.target.value)
    }

    async function insertWaitlist() {
        setProcessing(true)

        var emails = db.collection("waitlist").where("email", "==", email)
        const exists = await emails.get().then(function (snapshot: any) {
            return !snapshot.empty
        })

        if (!exists) {
            db.collection("waitlist")
                .doc(email)
                .set({
                    name: name,
                    email: email,
                    referrals: 0,
                    points: INITIAL_POINTS,
                    google_auth_email: "",
                })
                .then(() =>
                    dispatch(
                        insertWaitlistAction(email, name, INITIAL_POINTS, 0)
                    )
                )
                .then(() => history.push("/application"))
        } else {
            db.collection("waitlist")
                .doc(email)
                .get()
                .then(function (snapshot) {
                    let document = snapshot.data()
                    if (document) {
                        dispatch(
                            insertWaitlistAction(
                                email,
                                document.name,
                                document.points,
                                0
                            )
                        )
                    }
                })
                .then(() => setProcessing(false))
        }
    }

    return (
        <div>
            <div
                style={{
                    width: 800,
                    margin: "auto",
                    marginTop: 20,
                    display: "flex",
                    justifyContent: "space-between",
                }}
            >
                <input
                    type="text"
                    placeholder="Email Address"
                    onChange={updateEmail}
                    className="waitlist-form"
                    style={{ width: 180 }}
                />
                <input
                    type="text"
                    placeholder="Name"
                    onChange={updateName}
                    className="waitlist-form"
                    style={{ width: 180 }}
                />
                <input
                    type="text"
                    placeholder="Referral Code"
                    onChange={updateReferralCode}
                    className="waitlist-form"
                    style={{ width: 180 }}
                />
                <CountryDropdown
                    value={country}
                    onChange={(country) => updateCountry(country)}
                />
            </div>
            <div style={{ width: 800, margin: "auto", marginTop: 20 }}>
                {user && user.email ? (
                    <Button
                        className="waitlist-button"
                        disabled={true}
                        style={{
                            opacity: 1.0,
                        }}
                    >
                        You're on the waitlist as {user.name}.
                    </Button>
                ) : !processing ? (
                    <Button
                        onClick={insertWaitlist}
                        className="waitlist-button"
                        disabled={email && name && country ? false : true}
                        style={{
                            opacity: email && name && country ? 1.0 : 0.5,
                        }}
                    >
                        REQUEST ACCESS
                    </Button>
                ) : (
                    <Button
                        className="waitlist-button"
                        disabled={true}
                        style={{
                            opacity: email && name && country ? 1.0 : 0.5,
                        }}
                    >
                        Processing
                    </Button>
                )}
            </div>
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(WaitlistForm)
