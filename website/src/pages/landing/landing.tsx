import React, { useEffect, useCallback } from "react"
import { connect } from "react-redux"
import TypeWriterEffect from "react-typewriter-effect"
import { Row, Col, Button } from "react-bootstrap"

import { db } from "utils/firebase"
import * as firebase from "firebase"

import {
    updateUserAction,
    updateWaitlistAction,
} from "store/actions/auth/waitlist"

import "styles/landing.css"

import WaitlistForm from "pages/landing/components/waitlistForm"
import CountdownTimer from "pages/landing/components/countdown"
import Leaderboard from "pages/landing/components/leaderboard"
import Actions from "pages/landing/components/actions"

import LaptopAwe from "assets/largeGraphics/laptopAwe.svg"
import Moon from "assets/largeGraphics/moon.svg"
import Mars from "assets/largeGraphics/mars.svg"
import Mercury from "assets/largeGraphics/mercury.svg"
import Saturn from "assets/largeGraphics/saturn.svg"
import Plants from "assets/largeGraphics/plants.svg"
import Wireframe from "assets/largeGraphics/wireframe.svg"
import Mountain from "assets/largeGraphics/mountain.svg"

function Landing(props: any) {
    const { dispatch, user } = props

    const getRanking = useCallback(
        (waitlist: any[]) => {
            for (var i = 0; i < waitlist.length; i++) {
                if (waitlist[i].email === user.email) {
                    return i + 1
                }
            }
            return 0
        },
        [user]
    )

    useEffect(() => {
        console.log("use effect landing")
        var unsubscribe: any

        const updateRanking = (
            userData: firebase.firestore.DocumentData | undefined,
            waitlist: any[]
        ) => {
            // Bubbles user up the leaderboard according to new points
            if (userData) {
                for (
                    let currRanking = user.ranking - 1;
                    currRanking > 0 &&
                    userData.points >= waitlist[currRanking - 1].points;
                    currRanking--
                ) {
                    if (userData.email > waitlist[currRanking - 1].email) {
                        return currRanking + 1
                    }
                }
            }
            return -1
        }

        getWaitlist()
            .then((waitlist) => {
                dispatch(updateWaitlistAction(waitlist))
                if (user.email) {
                    const ranking = getRanking(waitlist)
                    if (ranking !== user.ranking) {
                        dispatch(updateUserAction(user.points, ranking))
                    }
                }
                return waitlist
            })
            .then((waitlist) => {
                if (user && user.email) {
                    unsubscribe = db
                        .collection("waitlist")
                        .doc(user.email)
                        .onSnapshot(
                            (doc: any) => {
                                console.log("IN SNAPSHOT")
                                const userData = doc.data()
                                const ranking = updateRanking(
                                    userData,
                                    waitlist
                                )
                                console.log(ranking)
                                if (
                                    userData &&
                                    userData.points !== user.points &&
                                    ranking !== -1
                                ) {
                                    dispatch(
                                        updateUserAction(
                                            userData.points,
                                            ranking
                                        )
                                    )
                                }
                            },
                            (err: Error) => console.log(err)
                        )
                }
            })
        return unsubscribe
    }, [dispatch, user, getRanking])

    async function getWaitlist() {
        const waitlist = await db
            .collection("waitlist")
            .orderBy("points", "desc")
            .orderBy("email")
            .get()
        return waitlist.docs.map((doc: any) => doc.data())
    }

    return (
        <div style={{ paddingBottom: 100 }}>
            <div
                className="banner-background"
                style={{ width: "100vw", position: "relative", zIndex: 1 }}
            >
                <img
                    src={Moon}
                    alt=""
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
                    alt=""
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
                    alt=""
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
                    alt=""
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
                    alt=""
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
                    alt=""
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
                    alt=""
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
                            multiText={["Blender", "Figma", "VSCode", "Maya"]}
                            loop={true}
                            typeSpeed={200}
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
                        maxWidth: 1000,
                        width: "45vw",
                        position: "relative",
                        bottom: 60,
                        zIndex: 100,
                        textAlign: "center",
                    }}
                >
                    <img
                        src={LaptopAwe}
                        alt=""
                        style={{ maxWidth: 1000, width: "100%", zIndex: 100 }}
                    />
                </div>
            </div>
            <div style={{ position: "relative", background: "white" }}>
                <Row style={{ paddingTop: 50, paddingRight: 50 }}>
                    <Col md={7}>
                        <img src={Wireframe} alt="" style={{ width: "100%" }} />
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
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Landing)
