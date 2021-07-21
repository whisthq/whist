/* eslint-disable react/jsx-props-no-spreading */
import React, { useEffect } from "react"
import ReactGA from "react-ga"
import { config } from "@app/shared/constants/config"

if (import.meta.env.MODE === "production") {
    // optional add gaOptions : {a bunch of options} such as debug and so forth... this should go in config
    ReactGA.initialize(
        config.keys.GOOGLE_ANALYTICS_TRACKING_CODES.map(
            (trackingId: string) => {
                return {
                    trackingId: trackingId,
                }
            }
        )
    )
    ReactGA.pageview(window.location.pathname + window.location.search)
}

// this will wrap a component (i.e. it takes as an input a component and adds some functionality to it)
// to turn it into a google analytics tracking page
const withTracker = (WrappedComponent: any, options = {}) => {
    const trackPage = (page: any) => {
        ReactGA.set({
            page,
            ...options,
        })
        ReactGA.pageview(page)
    }

    const HOC = (props: any) => {
        // every time the props change render the page
        useEffect(() => {
            const {
                location: { pathname: page },
            } = props
            trackPage(page)
        }, [props]) // only run on the first i.e. component mount

        return <WrappedComponent {...props} />
    }

    return HOC
}

export default withTracker
