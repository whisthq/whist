import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import TypeWriterEffect from "react-typewriter-effect";
import { Row, Col, Button } from "react-bootstrap";

import { db } from "utils/firebase";

import {
    updateUserAction,
    updateWaitlistAction,
} from "store/actions/auth/waitlist";

import "styles/landing.css";

import WaitlistForm from "pages/landing/components/waitlistForm";
import CountdownTimer from "pages/landing/components/countdown";
import Leaderboard from "pages/landing/components/leaderboard";
import Actions from "pages/landing/components/actions";

import LaptopAwe from "assets/largeGraphics/laptopAwe.svg";
import Moon from "assets/largeGraphics/moon.svg";
import Mars from "assets/largeGraphics/mars.svg";
import Mercury from "assets/largeGraphics/mercury.svg";
import Saturn from "assets/largeGraphics/saturn.svg";
import Plants from "assets/largeGraphics/plants.svg";
import Wireframe from "assets/largeGraphics/wireframe.svg";
import Mountain from "assets/largeGraphics/mountain.svg";

function Landing(props: any) {
    const [waitlist, setWaitlist] = useState(props.waitlist);

    useEffect(() => {
        console.log("use effect landing");
        console.log(props);
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
            .orderBy("email")
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
            userData.points >= props.waitlist[currRanking - 1].points
        ) {
            if (userData.email > props.waitlist[currRanking - 1].email) {
                break;
            }
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
                style={{ width: "100vw", position: "relative", zIndex: 1 }}
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
                <img
                    src={Mountain}
                    style={{
                        position: "absolute",
                        width: "100vw",
                        bottom: 150,
                        left: 0,
                        zIndex: 1,
                    }}
                />
                <div
                    style={{
                        position: "absolute",
                        width: "100vw",
                        bottom: 0,
                        left: 0,
                        height: 150,
                        background: "white",
                    }}
                ></div>
                <img
                    src={Plants}
                    style={{
                        position: "absolute",
                        width: 250,
                        left: 0,
                        bottom: 130,
                        zIndex: 2,
                    }}
                />
                <img
                    src={Plants}
                    style={{
                        position: "absolute",
                        width: 250,
                        right: 0,
                        bottom: 130,
                        transform: "scaleX(-1)",
                        zIndex: 2,
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
                            multiText={[
                                "Blender",
                                "Figma",
                                "VSCode",
                                "Maya",
                                "Blender",
                            ]}
                            typeSpeed={150}
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
                        zIndex: 100,
                    }}
                >
                    <img
                        src={LaptopAwe}
                        style={{ maxWidth: 600, zIndex: 100 }}
                    />
                </div>
            </div>
            <div style={{ position: "relative", background: "white" }}>
                <Row style={{ paddingTop: 50, paddingRight: 50 }}>
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
                            Run demanding apps on any computer or tablet
                        </div>
                        <div style={{ marginTop: 40 }}>
                            Fractal is like Google Stadia for creative and
                            productivity apps. With Fractal, you can run VSCode
                            on an iPad or create 3D animations on a Chromebook.
                        </div>
                    </Col>
                </Row>
            </div>
            <div style={{ padding: 30, marginTop: 100 }}>
                <Row>
                    <Col md={6}>
                        <div
                            style={{
                                padding: "50px 35px",
                                display: "flex",
                                justifyContent: "space-between",
                                background: "rgba(221, 165, 248, 0.2)",
                                borderRadius: 5,
                            }}
                        >
                            <div
                                style={{
                                    fontSize: 35,
                                    fontWeight: "bold",
                                    lineHeight: 1.3,
                                }}
                            >
                                Q:
                            </div>
                            <div style={{ width: 25 }}></div>
                            <div>
                                <div
                                    style={{
                                        fontSize: 35,
                                        fontWeight: "bold",
                                        lineHeight: 1.3,
                                        paddingBottom: 10,
                                    }}
                                >
                                    How can I be invited to try Fractal?
                                </div>
                                <div style={{ paddingTop: 20 }}>
                                    When the countdown hits zero, we’ll invite{" "}
                                    <strong>20 people</strong> from the waitlist
                                    with the most compelling{" "}
                                    <strong>100-word submission</strong> on why
                                    they want Fractal to receive 1:1 onboarding.
                                    We'll also invite the top{" "}
                                    <strong>20 people</strong> with the
                                </div>
                            </div>
                        </div>
                        <div
                            style={{
                                padding: "5px 30px",
                                background: "rgba(172, 207, 231, 0.2)",
                                borderRadius: 5,
                                marginTop: 25,
                            }}
                        >
                            <div
                                style={{
                                    fontWeight: "bold",
                                    fontSize: 125,
                                    height: 120,
                                }}
                            >
                                "
                            </div>
                            <div>
                                F*CKING AMAZING ... tried Minecraft VR - it was
                                buttery smooth with almost no detectable
                                latency. Never thought it would work this well.
                            </div>
                            <div
                                style={{
                                    fontWeight: "bold",
                                    fontSize: 125,
                                    textAlign: "right",
                                    height: 100,
                                    position: "relative",
                                    bottom: 20,
                                }}
                            >
                                "
                            </div>
                            <div style={{ position: "relative", bottom: 30 }}>
                                <div style={{ fontWeight: "bold" }}>
                                    Sean S.
                                </div>
                                <div style={{ fontSize: 12 }}>
                                    Designer + VR user
                                </div>
                            </div>
                        </div>
                    </Col>
                    <Col md={6}>
                        <div
                            style={{
                                background: "rgba(172, 207, 231, 0.2)",
                                padding: "5px 30px",
                                borderRadius: 5,
                            }}
                        >
                            <div
                                style={{
                                    fontWeight: "bold",
                                    fontSize: 125,
                                    height: 120,
                                }}
                            >
                                "
                            </div>
                            <div>
                                Woah! What you've developed is something I've
                                been dreaming of for years now... this is a
                                MASSIVE game changer for people that do the kind
                                of work that I do.
                            </div>
                            <div
                                style={{
                                    fontWeight: "bold",
                                    fontSize: 125,
                                    textAlign: "right",
                                    height: 100,
                                    position: "relative",
                                    bottom: 20,
                                }}
                            >
                                "
                            </div>
                            <div style={{ position: "relative", bottom: 30 }}>
                                <div style={{ fontWeight: "bold" }}>
                                    Brian M.
                                </div>
                                <div style={{ fontSize: 12 }}>Animator</div>
                            </div>
                        </div>
                        <div
                            style={{
                                padding: "5px 30px",
                                background: "rgba(172, 207, 231, 0.2)",
                                borderRadius: 5,
                                marginTop: 25,
                            }}
                        >
                            <div
                                style={{
                                    fontWeight: "bold",
                                    fontSize: 125,
                                    height: 120,
                                }}
                            >
                                "
                            </div>
                            <div>
                                [Fractal] is an immense project with a fantastic
                                amount of potential.
                            </div>
                            <div
                                style={{
                                    fontWeight: "bold",
                                    fontSize: 125,
                                    textAlign: "right",
                                    height: 100,
                                    position: "relative",
                                    bottom: 20,
                                }}
                            >
                                "
                            </div>
                            <div style={{ position: "relative", bottom: 30 }}>
                                <div style={{ fontWeight: "bold" }}>
                                    Jonathan H.
                                </div>
                                <div style={{ fontSize: 12 }}>
                                    Software developer + gamer
                                </div>
                            </div>
                        </div>
                    </Col>
                </Row>
            </div>
            <div
                style={{
                    padding: 30,
                    marginTop: 100,
                    background: "#0d1d3c",
                    minHeight: "100vh",
                }}
            >
                <Row>
                    <Col md={6}>
                        <Leaderboard />
                    </Col>
                    <Col md={6}>
                        <Actions />
                    </Col>
                </Row>
            </div>
            <div style={{ padding: 30 }}>
                <div
                    style={{
                        borderRadius: 5,
                        boxShadow: "0px 4px 30px rgba(0, 0, 0, 0.1)",
                        padding: "60px 100px",
                        background: "white",
                    }}
                >
                    <div
                        style={{
                            maxWidth: 650,
                            margin: "auto",
                            textAlign: "left",
                        }}
                    >
                        <div
                            style={{
                                fontSize: 40,
                                fontWeight: "bold",
                                lineHeight: 1.4,
                            }}
                        >
                            Give my computer superpowers.
                        </div>
                        <div style={{ marginTop: 20 }}>
                            Run the most demanding applications from my device
                            without eating up your RAM or processing power, all
                            on 1GB datacenter internet.
                        </div>
                        <Button
                            style={{
                                marginTop: 30,
                                background: "#1C2A45",
                                fontWeight: "bold",
                                border: "none",
                                padding: "10px 40px",
                            }}
                        >
                            I Want Access
                        </Button>
                    </div>
                </div>
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
