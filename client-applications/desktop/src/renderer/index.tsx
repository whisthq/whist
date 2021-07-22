/**
 * Copyright Fractal Computers, Inc. 2021
 * @file index.tsx
 * @brief This file is the entry point of the renderer thread and acts as a router.
 */

import React from "react"
import { chain, keys } from "lodash"
import ReactDOM from "react-dom"

import Update from "@app/renderer/pages/update"
import Typeform from "@app/renderer/pages/typeform"
import { OneButtonError, TwoButtonError } from "@app/renderer/pages/error"
import Signout from "@app/renderer/pages/signout"

import {
    WindowHashUpdate,
    WindowHashSignout,
    WindowHashTypeform,
    allowPayments,
} from "@app/utils/constants"
import {
    fractalError,
    NO_PAYMENT_ERROR,
    UNAUTHORIZED_ERROR,
    AUTH_ERROR,
    NAVIGATION_ERROR,
} from "@app/utils/error"
import { useMainState } from "@app/utils/ipc"
import TRIGGER from "@app/utils/triggers"

// Electron has no way to pass data to a newly launched browser
// window. To avoid having to maintain multiple .html files for
// each kind of window, we pass a constant across to the renderer
// thread as a query parameter to determine which component
// should be rendered.

// If no query parameter match is found, we default to a
// generic navigation error window.
const show = chain(window.location.search.substring(1))
    .split("=")
    .chunk(2)
    .fromPairs()
    .get("show")
    .value()

const RootComponent = () => {
    const [, setMainState] = useMainState()
    const relaunch = () =>
        setMainState({
            trigger: { name: TRIGGER.relaunchAction, payload: undefined },
        })

    const showPaymentWindow = () =>
        setMainState({
            trigger: { name: TRIGGER.showPaymentWindow, payload: undefined },
        })

    const clearCache = () =>
        setMainState({
            trigger: { name: TRIGGER.clearCacheAction, payload: undefined },
        })

    const showSignoutWindow = () =>
        setMainState({
            trigger: { name: TRIGGER.showSignoutWindow, payload: undefined },
        })

    const onTypeformSubmit = () => {
        setMainState({
            trigger: { name: TRIGGER.persistTypeform, payload: null },
        })
    }

    if (show === WindowHashUpdate) return <Update />
    if (show === WindowHashSignout) return <Signout onClick={clearCache} />
    if (show === WindowHashTypeform)
        return <Typeform onSubmit={onTypeformSubmit} />
    if (show === NO_PAYMENT_ERROR && allowPayments)
        return (
            <TwoButtonError
                title={fractalError[show].title}
                text={fractalError[show].text}
                primaryButtonText="Update Payment"
                secondaryButtonText="Sign Out"
                onPrimaryClick={showPaymentWindow}
                onSecondaryClick={showSignoutWindow}
            />
        )
    if ([UNAUTHORIZED_ERROR, AUTH_ERROR, NAVIGATION_ERROR].includes(show))
        return (
            <OneButtonError
                title={fractalError[show].title}
                text={fractalError[show].text}
                primaryButtonText="Sign Out"
                onPrimaryClick={showSignoutWindow}
            />
        )
    if (keys(fractalError).includes(show))
        return (
            <TwoButtonError
                title={fractalError[show].title}
                text={fractalError[show].text}
                primaryButtonText="Try Again"
                secondaryButtonText="Sign Out"
                onPrimaryClick={relaunch}
                onSecondaryClick={showSignoutWindow}
            />
        )
    return (
        <TwoButtonError
            title={fractalError.NAVIGATION_ERROR.title}
            text={fractalError.NAVIGATION_ERROR.text}
            primaryButtonText="Try Again"
            secondaryButtonText="Sign Out"
            onPrimaryClick={relaunch}
            onSecondaryClick={showSignoutWindow}
        />
    )
}

// TODO: actually pass version number through IPC.
const WindowBackground = (props: any) => {
    return (
        <div className="relative w-full h-full bg-white">{props.children}</div>
    )
}

ReactDOM.render(
    <WindowBackground>
        <RootComponent />
    </WindowBackground>,
    document.getElementById("root")
)
