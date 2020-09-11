import React from "react";

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

const Actions = () => {
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
            <SignupAction />
            <RecommendAction />
        </div>
    );
};

export default Actions;
