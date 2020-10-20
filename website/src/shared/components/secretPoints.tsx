import React from "react"
import { useMutation } from "@apollo/client"
import { connect } from "react-redux"

import "styles/shared.css"

import {
    giveSecretPointsAction,
} from "store/actions/waitlist/sideEffects"
import { if_exists_else } from "shared/utils/reducerHelpers"
import { UPDATE_WAITLIST } from "shared/constants/graphql"

const emptySet = new Set()

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
function SecretPoints(props: any) {
    const [updatePoints] = useMutation(UPDATE_WAITLIST, {
        onError(err) {},
    })

    async function handleClick() {
        props.dispatch(giveSecretPointsAction(props.name))

        // rank is updated in the waitlist, local waitlist is subscribed so recieves update
        // this updates local user values
        updatePoints({
            variables: {
                user_id: props.user.user_id,
                points: props.user.points + props.points,
            },
            optimisticResponse: true,
        })
    }

    // TODO style this!
    return props.name && props.available_secret_points.has(props.name) &&
        props.points ? (
        props.renderProps ? (
            props.renderProps(handleClick)
        ) : (
            <div
                onClick={handleClick}
                style={props.style ? props.style : { width: "50%" }}
                className={props.className ? props.className : "pointer-hover"}
            >
                Click me for{" "}
                <span
                    style={
                        props.spanStyle ? props.spanStyle : { color: "#3930b8" }
                    }
                >
                    {" "}
                    {props.points} Points{" "}
                </span>
            </div>
        )
    ) : (
        <div />
    )
}

function mapStateToProps(state: {
    AuthReducer: {
        user: any
        waitlist: any
        closing_date: any
        available_secret_points: Set<string>
    }
}) {
    return {
        waitlist: state.AuthReducer.waitlist,
        available_secret_points: if_exists_else(state.AuthReducer, ["waitlistUser", "available_secret_points"], emptySet),
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(SecretPoints)