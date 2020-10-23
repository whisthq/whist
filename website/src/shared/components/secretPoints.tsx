import React from "react"
import { useMutation } from "@apollo/client"
import { connect } from "react-redux"

import "styles/shared.css"

import { deep_copy } from "shared/utils/reducerHelpers"
import { UPDATE_WAITLIST } from "shared/constants/graphql"
import { updateWaitlistUser } from "store/actions/waitlist/pure"

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
    user: any
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
        user,
        points,
        renderProps,
        name,
        style,
        className,
        spanStyle,
    } = props

    const [updatePoints] = useMutation(UPDATE_WAITLIST, {
        onError(err) {},
    })

    async function handleClick() {
        const newEastereggs = deep_copy(
            waitlistUser.eastereggsAvailable
                ? waitlistUser.eastereggsAvailable
                : {}
        )
        newEastereggs[name] = undefined

        dispatch(
            updateWaitlistUser({
                eastereggsAvailable: newEastereggs,
            })
        )

        // rank is updated in the waitlist, local waitlist is subscribed so recieves update
        // this updates local user values
        updatePoints({
            variables: {
                user_id: user.user_id,
                points: waitlistUser.points + points,
            },
            optimisticResponse: true,
        })
    }

    console.log(waitlistUser)
    console.log(waitlistUser.eastereggsAvailable)

    return name &&
        waitlistUser.eastereggsAvailable &&
        waitlistUser.eastereggsAvailable[name] &&
        points ? (
        renderProps ? (
            renderProps(handleClick)
        ) : (
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
        )
    ) : (
        <div />
    )
}

function mapStateToProps(state: {
    WaitlistReducer: {
        waitlistUser: { points: number; eastereggsAvailable: any }
    }
    AuthReducer: { user: { user_id: string } }
}) {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        waitlistUser: state.WaitlistReducer.waitlistUser
            ? state.WaitlistReducer.waitlistUser
            : {},
    }
}

export default connect(mapStateToProps)(SecretPoints)
