import React from "react";

import "styles/landing.css";

import Leaderboard from "pages/landing/components/leaderboard";
import Actions from "pages/landing/components/actions";

const LeaderboardPage = () => {
    return (
        <div className="leaderboardPage">
            <Leaderboard />
            <Actions />
        </div>
    );
};

export default LeaderboardPage;
