import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { FormControl, InputGroup, Button } from "react-bootstrap";
import { CountryDropdown } from 'react-country-region-selector';

import { db } from "utils/firebase";

import { insertWaitlistAction } from "store/actions/auth/waitlist"

import "styles/landing.css";

function WaitlistForm(props: any) {
    const [state, setState] = useState(() => {
        return ({
            "email": null,
            "name": null,
            "country": "United States"
        })
    })

    useEffect(() => {
        console.log(props.user)
    });

    function updateEmail(evt: any) {
        evt.persist();
        setState(prevState => { return { ...prevState, "email": evt.target.value } })
    }

    function updateName(evt: any) {
        evt.persist();
        setState(prevState => { return { ...prevState, "name": evt.target.value } })
    }

    function updateCountry(country: string) {
        setState(prevState => { return { ...prevState, "country": country } })
    }

    async function insertWaitlist() {
        var emails = db.collection("waitlist").where('email', '==', state.email);
        const exists = await emails.get().then(function (snapshot) {
            return !snapshot.empty
        });

        if (!exists) {
            db.collection("waitlist").add({
                name: state.name,
                email: state.email,
                referrals: 0,
                points: 50,
            });
            insertWaitlistAction(state.email, state.name);
        } else {
            console.log("ALREADY EXISTS")
        }
    }

    return (
        <div>
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
                value={state.country}
                onChange={(country) => updateCountry(country)}
            />
            <div>
                <Button
                    onClick={insertWaitlist}
                    className="waitlist-button"
                    disabled={state.email && state.name && state.country ? false : true}
                    style={{
                        opacity: state.email && state.name && state.country ? 1.0 : 0.5
                    }}
                >
                    Submit
                </Button>
            </div>
        </div >
    );
}

function mapStateToProps(state) {
    return {
        user: state.AuthReducer.user
    }
}

export default connect(mapStateToProps)(WaitlistForm);
