import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import TypeWriterEffect from "react-typewriter-effect";
import { Row, Col } from "react-bootstrap";

import { db } from "utils/firebase";

import {
    updateUserAction,
    updateWaitlistAction,
} from "store/actions/auth/waitlist";

import "styles/landing.css";

import WaitlistForm from "pages/landing/components/waitlistForm";
import CountdownTimer from "pages/landing/components/countdown";
import LeaderboardPage from "pages/landing/components/leaderboardPage";

import LaptopAwe from "assets/largeGraphics/laptopAwe.svg";
import Moon from "assets/largeGraphics/moon.svg";
import Mars from "assets/largeGraphics/mars.svg";
import Mercury from "assets/largeGraphics/mercury.svg";
import Saturn from "assets/largeGraphics/saturn.svg";
import Plants from "assets/largeGraphics/plants.svg";
import Wireframe from "assets/largeGraphics/wireframe.svg";

function Landing(props: any) {
    const [waitlist, setWaitlist] = useState(props.waitlist);

    useEffect(() => {
        console.log("use effect landing");
        console.log(props.user);
        var unsubscribe;
        getWaitlist()
            .then((waitlist) => {
                setWaitlist(waitlist);
                props.dispatch(updateWaitlistAction(waitlist));
                if (props.user.email && props.user.ranking == 0) {
                    const ranking = getInitialRanking(waitlist);
                    props.dispatch(
                        updateUserAction(props.user.points, ranking)
                    );
                }
            })
            .then(() => {
                if (props.user && props.user.email) {
                    unsubscribe = db
                        .collection("waitlist")
                        .doc(props.user.email)
                        .onSnapshot((doc) => {
                            console.log("IN SNAPSHOT");
                            const userData = doc.data();
                            const ranking = updateRanking(userData);
                            console.log(ranking);
                            if (
                                userData &&
                                userData.points != props.user.points
                            ) {
                                props.dispatch(
                                    updateUserAction(userData.points, ranking)
                                );
                            }
                        });
                }
            });
        return unsubscribe;
    }, [props.user]);

    async function getWaitlist() {
        const waitlist = await db
            .collection("waitlist")
            .orderBy("points", "desc")
            .get();
        return waitlist.docs.map((doc) => doc.data());
    }

    function getInitialRanking(waitlist) {
        for (var i = 0; i < waitlist.length; i++) {
            if (waitlist[i].email == props.user.email) {
                return i + 1;
            }
        }
        return 0;
    }

    function updateRanking(userData) {
        var currRanking = props.user.ranking - 1;
        // Bubbles user up the leaderboard according to new points
        while (
            currRanking > 0 &&
            userData.points > props.waitlist[currRanking - 1].points
        ) {
            setWaitlist((originalWaitlist) => {
                var newWaitlist = [...originalWaitlist];
                newWaitlist[currRanking] = originalWaitlist[currRanking - 1];
                newWaitlist[currRanking - 1] = userData;
                return newWaitlist;
            });
            currRanking--;
        }
        return currRanking + 1;
    }

    return (
        <div style={{ paddingBottom: 100 }}>
            <div
                className="banner-background"
                style={{ zIndex: 2, width: "100vw" }}
            >
                <img
                    src={Moon}
                    style={{
                        position: "absolute",
                        width: 100,
                        height: 100,
                        top: 125,
                        left: 40,
                    }}
                />
                <img
                    src={Mars}
                    style={{
                        position: "absolute",
                        width: 120,
                        height: 120,
                        top: 185,
                        right: -40,
                    }}
                />
                <img
                    src={Mercury}
                    style={{
                        position: "absolute",
                        width: 80,
                        height: 80,
                        top: 405,
                        right: 90,
                    }}
                />
                <img
                    src={Saturn}
                    style={{
                        position: "absolute",
                        width: 100,
                        height: 100,
                        top: 425,
                        left: 80,
                    }}
                />
                <div
                    style={{
                        display: "flex",
                        justifyContent: "space-between",
                        width: "100%",
                        padding: 30,
                    }}
                >
                    <div className="logo">Fractal</div>
                    <div style={{ position: "relative", right: 50 }}>
                        <CountdownTimer />
                    </div>
                </div>
                <div
                    style={{
                        margin: "auto",
                        marginTop: 20,
                        marginBottom: 20,
                        color: "white",
                        textAlign: "center",
                        width: 800,
                    }}
                >
                    <div
                        style={{
                            display: "flex",
                            margin: "auto",
                            justifyContent: "center",
                        }}
                    >
                        <TypeWriterEffect
                            textStyle={{
                                fontFamily: "Maven Pro",
                                color: "#00D4FF",
                                fontSize: 70,
                                fontWeight: "bold",
                                marginTop: 10,
                            }}
                            startDelay={0}
                            cursorColor="white"
                            multiText={["Blender", "Figma", "VSCode", "Maya"]}
                            typeSpeed={150}
                            loop={true}
                        />
                        <div
                            style={{
                                fontWeight: "bold",
                                fontSize: 70,
                                paddingBottom: 40,
                            }}
                        >
                            , just{" "}
                            <span style={{ color: "#00D4FF" }}>faster</span>.
                        </div>
                    </div>
                    <div
                        style={{
                            fontSize: 15,
                            width: 600,
                            margin: "auto",
                            lineHeight: 1.55,
                            color: "#EFEFEF",
                            letterSpacing: 1,
                        }}
                    >
                        Fractal uses cloud streaming to supercharge your
                        laptop's applications. <br /> Say goodbye to laggy apps
                        — join our waitlist before the countdown ends for
                        access.
                    </div>
                    <div style={{ marginTop: 50 }}>
                        <WaitlistForm />
                    </div>
                </div>
                <div
                    style={{
                        margin: "auto",
                        maxWidth: 600,
                        position: "relative",
                        bottom: 60,
                        zIndex: 2,
                    }}
                >
                    <img src={LaptopAwe} style={{ maxWidth: 600 }} />
                </div>
            </div>
            <div
                className="white-background-curved"
                style={{
                    height: 350,
                    position: "relative",
                    bottom: 150,
                    background: "white",
                    zIndex: 1,
                }}
            >
                <img
                    src={Plants}
                    style={{
                        position: "absolute",
                        width: 250,
                        left: 0,
                        top: 10,
                    }}
                />
                <img
                    src={Plants}
                    style={{
                        position: "absolute",
                        width: 250,
                        right: 0,
                        top: 10,
                        transform: "scaleX(-1)",
                    }}
                />
                <Row style={{ paddingTop: 200, paddingRight: 50 }}>
                    <Col md={7}>
                        <img src={Wireframe} style={{ width: "100%" }} />
                    </Col>
                    <Col md={5} style={{ paddingLeft: 40 }}>
                        <div
                            style={{
                                fontSize: 50,
                                fontWeight: "bold",
                                lineHeight: 1.2,
                            }}
                        >
                            Unlock desktop apps on any computer or tablet
                        </div>
                        <div style={{ marginTop: 40 }}>
                            Fractal is like Google Stadia for all your apps. For
                            the first time, run VSCode on an iPad or create 3D
                            animations on a Chromebook.
                        </div>
                    </Col>
                </Row>
            </div>
            <div style={{ marginTop: 200 }}>
                <LeaderboardPage />
            </div>
        </div>
    );
}

function mapStateToProps(state) {
    return {
        user: state.AuthReducer.user,
        waitlist: state.AuthReducer.waitlist,
    };
}

export default connect(mapStateToProps)(Landing);
