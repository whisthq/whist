import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { FormControl, InputGroup, Button } from "react-bootstrap";
import { CountryDropdown } from "react-country-region-selector";

import { db } from "utils/firebase";

import { insertWaitlistAction } from "store/actions/auth/waitlist";

import "styles/landing.css";

const INITIAL_POINTS = 10;

function WaitlistForm(props: any) {
    const [email, setEmail] = useState();
    const [name, setName] = useState();
    const [country, setCountry] = useState("United States");
    const [referralCode, setReferralCode] = useState();

    useEffect(() => {
        console.log("Use Effect waitlist");
    }, []);

    function updateEmail(evt: any) {
        evt.persist();
        setEmail(evt.target.value);
    }

    function updateName(evt: any) {
        evt.persist();
        setName(evt.target.value);
    }

    function updateCountry(country: string) {
        setCountry(country);
    }

    function updateReferralCode(evt: any) {
        evt.persist();
        setReferralCode(evt.target.value);
    }

    async function insertWaitlist() {
        var emails = db.collection("waitlist").where("email", "==", email);
        const exists = await emails.get().then(function (snapshot) {
            return !snapshot.empty;
        });


        if (!exists) {
            db.collection("waitlist").doc(email).set({
                name: name,
                email: email,
                referrals: 0,
                points: INITIAL_POINTS,
            });
            props.dispatch(insertWaitlistAction(email, name, INITIAL_POINTS));
        }
    }

    return (
        <div>
            <div style={{ width: 800, margin: "auto", marginTop: 20, display: "flex", justifyContent: "space-between" }}>
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
            <div style={{ width: 800, margin: "auto", marginTop: 20, }}>
                <Button
                    onClick={insertWaitlist}
                    className="waitlist-button"
                    disabled={email && name && country ? false : true}
                    style={{
                        opacity: email && name && country ? 1.0 : 0.5
                    }}
                >
                    Submit
                </Button>
            </div>
        </div>
    );
}

function mapStateToProps(state) {
    return {
        user: state.AuthReducer.user,
    };
}

export default connect(mapStateToProps)(WaitlistForm);
