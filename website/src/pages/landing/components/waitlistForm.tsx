import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { FormControl, InputGroup, Button } from "react-bootstrap";
import { CountryDropdown } from "react-country-region-selector";

import { db } from "utils/firebase";

import { insertWaitlistAction } from "store/actions/auth/waitlist";

import "styles/landing.css";

const INITIAL_POINTS = 50;

function WaitlistForm(props: any) {
    const [email, setEmail] = useState();
    const [name, setName] = useState();
    const [country, setCountry] = useState("United States");

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
        <div style={{ maxHeight: "100vh", padding: 200 }}>
            <div className="waitlist-form-container">
                <InputGroup className="mb-3" style={{ marginTop: 20 }}>
                    <FormControl
                        type="email"
                        aria-label="Default"
                        aria-describedby="inputGroup-sizing-default"
                        placeholder="Email Address"
                        onChange={updateEmail}
                        className="waitlist-form"
                    />
                    <br />
                </InputGroup>
                <InputGroup className="mb-3" style={{ marginTop: 20 }}>
                    <FormControl
                        type="email"
                        aria-label="Default"
                        aria-describedby="inputGroup-sizing-default"
                        placeholder="Name"
                        onChange={updateName}
                        className="waitlist-form"
                    />
                    <br />
                </InputGroup>
                <CountryDropdown
                    value={country}
                    onChange={(country) => updateCountry(country)}
                />
                <div>
                    <Button
                        onClick={insertWaitlist}
                        className="waitlist-button"
                        disabled={email && name && country ? false : true}
                        style={{
                            opacity: email && name && country ? 1.0 : 0.5,
                        }}
                    >
                        Submit
                    </Button>
                </div>
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
