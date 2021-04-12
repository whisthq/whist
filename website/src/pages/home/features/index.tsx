import React from "react"
import { Row } from "react-bootstrap"

import Lock from "@app/assets/icons/lock.svg"
import Cloud from "@app/assets/icons/cloud.svg"
import Rocket from "@app/assets/icons/rocket.svg"
import FeatureBox from "./featureBox"
// import FractalPeriod from "assets/icons/fractalPeriod.svg"

export const Features = () => {
    /*
        Features section of Chrome product page
 
        Arguments: 
            none
    */

    return (
        <div className="mt-32 text-center">
            <div className="pt-24">
                <div className="text-3xl md:text-5xl dark:text-gray-300 leading-normal">
                    The first{" "}
                    <span className="text-blue dark:text-mint">
                        cloud-powered
                    </span>{" "}
                    browser
                </div>
            </div>
            <Row className="m-auto max-w-screen-lg pt-24">
                <FeatureBox
                    icon={
                        <img
                            src={Cloud}
                            className="relative w-10 h-10 bg-blue rounded p-2"
                            alt="cloud"
                        />
                    }
                    title="Cloud Storage Sync"
                    text="Fractal syncs with your Google Drive account so you can download files directly to Google Drive."
                />
                <FeatureBox
                    icon={
                        <img
                            src={Lock}
                            className="relative w-10 h-10 bg-blue rounded p-2"
                            alt="lock"
                        />
                    }
                    title="Security and Privacy"
                    text="Fractal never tracks your sessions, and all
                            information sent over the Internet is end-to-end AES
                            encrypted."
                />
                <FeatureBox
                    icon={
                        <img
                            src={Rocket}
                            className="relative w-10 h-10 bg-blue rounded p-2"
                            alt="rocket"
                        />
                    }
                    title="Lightning Speeds"
                    text="Fractal’s proprietary streaming technology runs at 60+ FPS with no noticeable lag."
                />
            </Row>
        </div>
    )
}

export default Features
