import React, { useEffect, useState, useContext } from "react";
import { Button } from "react-bootstrap";
import { connect } from "react-redux";
import * as firebase from "firebase";

import { googleLogin, logout } from "store/actions/auth/login_actions";

import "styles/auth.css";

const Auth = (props) => {
    const [error, setError] = useState();

    useEffect(() => {
        console.log(props);
    }, [props]);

    const handleGoogleLogin = () => {
        const provider = new firebase.auth.GoogleAuthProvider();

        firebase
            .auth()
            .signInWithPopup(provider)
            .then((result) => {
                const email = result.user.email;
                console.log(email);
                props.dispatch(googleLogin(email));
            })
            .catch((e) => setError(e));
    };

    const handleSignOut = () => {
        firebase
            .auth()
            .signOut()
            .then(() => {
                console.log("signed out");
                props.dispatch(logout());
            });
    };

    return (
        <div className="auth-wrapper">
            <div>Logged in: {JSON.stringify(props.loggedIn)}</div>
            <div>email: {props.email}</div>
            <Button onClick={handleGoogleLogin} className="google-button">
                Sign in with Google
            </Button>
            <Button onClick={handleSignOut} className="signout-button">
                Sign out
            </Button>
        </div>
    );
};

const mapStateToProps = (state) => {
    return {
        reducer: state.AuthReducer,
        loggedIn: state.AuthReducer.logged_in,
        email: state.AuthReducer.user.email,
        name: state.AuthReducer.user.name,
    };
};

export default connect(mapStateToProps)(Auth);
