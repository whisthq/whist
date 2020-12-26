import React, { useState, useEffect } from "react"
import { useMutation } from "@apollo/client"
import { connect } from "react-redux"

import "styles/shared.css"

import { UPDATE_WAITLIST_REFERRALS } from "shared/constants/graphql"
import { updateWaitlistUser } from "store/actions/waitlist/pure"
import { db } from "shared/utils/firebase"

/*
secret points is a one-time use button that will give the user points if they click it, but it's supposed to be
hidden somewhere in the site that you may not expect to encourage users to interact more with our stuff
*/

// points is how much points this credits you, name is the name of this specific tidbit (i.e. can call it like "header" if its in the header)
// takes in as props: points={number}, name={string} and you can also
// pass renderProps={(onClick: () => {...}) => jsx that onClicks the onClick}
// you can also instead pass style={{/*dict*/}} if you just want to style the div
// you can also pass in classname
// you can also pass in a span style for the style of the span element inside which has the 100 points text
function SecretPoints(props: {
    dispatch: (action: any) => any
    waitlistUser: any
    points: number
    renderProps?: any
    name: string
    style?: any
    className?: string
    spanStyle?: any
}) {
    const {
        dispatch,
        waitlistUser,
        points,
        renderProps,
        name,
        style,
        className,
        spanStyle,
    } = props

    const [updatePoints] = useMutation(UPDATE_WAITLIST_REFERRALS, {
        onCompleted: () => {
            setCompleted(true)
        },
        onError: () => {
            setError(true)
        },
    })
    const [newEasterEggs, setNewEasterEggs] = useState({})
    const [completed, setCompleted] = useState(false)
    const [error, setError] = useState(false)

    useEffect(() => {
        // if successfully updated database with new point, update redux and firebase
        if (completed && !error) {
            // update the local state which is used to decide which buttons to display
            dispatch(
                updateWaitlistUser({
                    eastereggsAvailable: newEasterEggs,
                })
            )

            // update remote state which tells us what the "truth" really is
            db.collection("eastereggs").doc(waitlistUser.userID).set({
                available: newEasterEggs,
            })
        }
    }, [completed, error, newEasterEggs, dispatch, waitlistUser.userID])

    async function handleClick() {
        setCompleted(false)
        setError(false)
        // get the real status from the firebase database
        const newEastereggsDocument = await db
            .collection("eastereggs")
            .doc(waitlistUser.userID)
            .get()
        const data = newEastereggsDocument.data()

        // remove the easteregg just clicked as long as the firebase is updated early enough
        // if the firebase is not yet updatedthen just don't do it
        // basically we don't want to do anything until we actually have them in the db since it will error out
        // the tradeoff is that the buttons don't work for the first couple seconds hopefully at worst
        if (data && data.available) {
            const newEastereggs = data.available
            const getPoints = name in newEastereggs
            delete newEastereggs[name] // pasing undefined not allowed
            setNewEasterEggs(newEastereggs)

            // rank is updated in the waitlist, local waitlist is subscribed so recieves update
            // this updates local user values
            if (getPoints) {
                updatePoints({
                    variables: {
                        userID: waitlistUser.userID,
                        points: waitlistUser.points + points,
                    },
                    optimisticResponse: true,
                })
            }
        }
    }

    return name &&
        waitlistUser.eastereggsAvailable &&
        waitlistUser.eastereggsAvailable[name] &&
        points ? (
        renderProps ? (
            renderProps(handleClick)
        ) : (
            <>
                <div
                    onClick={handleClick}
                    style={style ? style : { width: "50%" }}
                    className={className ? className : "pointer-hover"}
                >
                    Click me for{" "}
                    <span style={spanStyle ? spanStyle : { color: "#3930b8" }}>
                        {" "}
                        {points} Points{" "}
                    </span>
                </div>
                {error && (
                    <div style={{ color: "#ff0000" }}>Unable to add points</div>
                )}
            </>
        )
    ) : (
        <div />
    )
}

function mapStateToProps(state: {
    WaitlistReducer: {
        waitlistUser: {
            userID: string
            points: number
            eastereggsAvailable: any
        }
    }
}) {
    return {
        waitlistUser: state.WaitlistReducer.waitlistUser
            ? state.WaitlistReducer.waitlistUser
            : {},
    }
}

export default connect(mapStateToProps)(SecretPoints)
