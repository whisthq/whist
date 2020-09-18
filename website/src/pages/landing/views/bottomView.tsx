import React, { useContext } from "react"
import { Button } from "react-bootstrap"

import ScreenContext from "shared/context/screenContext"

import "styles/landing.css"

const BottomView = (props: any) => {
    const { width } = useContext(ScreenContext)

    const scrollToTop = () => {
        window.scrollTo({
            top: 0,
            behavior: "smooth",
        })
    }

    return (
        <div style={{ padding: 30, paddingRight: width > 720 ? 45 : 30 }}>
            <div
                style={{
                    borderRadius: 5,
                    padding: "60px 30px",
                    background: "rgba(91,71,212,0.08)",
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
                        Give my applications superpowers.
                    </h2>
                    <p style={{ marginTop: 20 }}>
                        Run the most demanding applications from my device using
                        10x less RAM and processing power, all on gigabyte
                        datacenter Internet.
                    </p>
                    <Button className="access-button" onClick={scrollToTop}>
                        REQUEST ACCESS
                    </Button>
                </div>
            </div>
        </div>
    )
}

export default BottomView
