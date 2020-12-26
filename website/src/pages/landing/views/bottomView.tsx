import React, { useContext } from "react"

import WaitlistForm from "shared/components/waitlistForm"
import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"

import "styles/landing.css"

const BottomView = (props: any) => {
    const { width, appHighlight } = useContext(MainContext)

    const myApplications = appHighlight ? appHighlight : "my applications"
    const theMostDemandingApplications = appHighlight
        ? appHighlight
        : "the most demanding applications"

    return (
        <div
            style={{
                borderRadius: 5,
                padding: width > ScreenSize.MEDIUM ? "60px 30px" : "50px 0",
                marginTop: width > ScreenSize.LARGE ? 75 : 25,
            }}
        >
            <div
                style={{
                    maxWidth: 650,
                    margin: "auto",
                    textAlign: "left",
                }}
            >
                <h2
                    style={{
                        fontSize: 40,
                        fontWeight: "bold",
                        lineHeight: 1.4,
                    }}
                >
                    Give {myApplications} superpowers.
                </h2>
                <p style={{ marginTop: 20 }}>
                    Run {theMostDemandingApplications} from my device using 10x
                    less RAM and processing power, all on gigabyte datacenter
                    Internet.
                </p>
                <div style={{ marginTop: 30 }}>
                    <WaitlistForm />
                </div>
            </div>
        </div>
    )
}

export default BottomView
