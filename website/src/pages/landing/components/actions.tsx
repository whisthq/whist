import React from "react";
import { connect } from "react-redux";
import { Button } from "react-bootstrap";

const SignupAction = () => {
    return (
        <div className="action">
            <div
                style={{
                    color: "white",
                    fontSize: "23px",
                }}
            >
                Sign up for a free account
            </div>
            <div style={{ color: "#00D4FF" }}> +100 points</div>
        </div>
    );
};

const RecommendAction = () => {
    return (
        <div className="action">
            <div
                style={{
                    color: "white",
                    fontSize: "23px",
                }}
            >
                Recommend a Friend
            </div>
            <div style={{ color: "#00D4FF" }}> +100 points</div>
        </div>
    );
};

const JoinWaitlistAction = () => {
    const scrollToTop = () => {
        window.scrollTo({
            top: 0,
            behavior: "smooth",
        });
    };

    return (
        <Button onClick={scrollToTop} className="action">
            <div
                style={{
                    color: "white",
                    fontSize: "23px",
                }}
            >
                Join Waitlist
            </div>
            <div style={{ color: "#00D4FF" }}> +50 points</div>
        </Button>
    );
};

const Actions = (props) => {
    const renderActions = () => {
        if (props.user && props.user.email) {
            if (props.loggedIn) {
                return <RecommendAction />;
            } else {
                return (
                    <>
                        <SignupAction /> <RecommendAction />
                    </>
                );
            }
        } else {
            return <JoinWaitlistAction />;
        }
    };

    return (
        <div
            style={{
                display: "flex",
                flexDirection: "column",
                alignItems: "flex-start",
                marginTop: "9vh",
            }}
        >
            <h1
                style={{
                    fontWeight: "bold",
                    color: "white",
                    marginBottom: "30px",
                    fontSize: "40px",
                }}
            >
                Actions
            </h1>
            {renderActions()}
        </div>
    );
};

const mapStateToProps = (state) => {
    return {
        user: state.AuthReducer.user,
        loggedIn: state.AuthReducer.logged_in,
    };
};

export default connect(mapStateToProps)(Actions);
