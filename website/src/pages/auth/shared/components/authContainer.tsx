// npm imports
import React, {useState, useEffect} from "react"
import { connect } from "react-redux"

// Component imports
import { PuffAnimation } from "shared/components/loadingAnimations"

// Function imports
import history from "shared/utils/history"
import { validateAccessToken } from "shared/api/index"
import { routeMap, fractalRoute } from "shared/constants/routes"

// Type + constant imports
import { User } from "shared/types/reducers"

// Asset imports
import LogoPurple from "assets/icons/logoPurple.png"

const AuthContainer = (props: {
    title?: string
    children: JSX.Element | JSX.Element[]
    user: User
}) => {
    const {title, children, user} = props
    const [processing, setProcessing] = useState(true)

    const goHome = () => {
        history.push("/")
    }

    // Check if the user's access token is valid. If it is, redirect to dashboard.
    useEffect(() => {
        if(user.accessToken) {
            setProcessing(true)
            validateAccessToken(user.accessToken).then(({json}) => {
                if(json && json.status === 200) {
                    history.push(fractalRoute(routeMap.DASHBOARD))
                }
                setProcessing(false)
            })
        } else {
            setProcessing(false)
        }
    }, [user.accessToken])

    // Render UI
    if(processing) {
        return(
            <div>
                <PuffAnimation />
            </div>
        )
    } else {
        return(
            <div className="w-96 m-auto relative top-32 min-h-screen">
                <img src={LogoPurple} className="w-12 h-12 m-auto cursor-pointer" onClick={goHome} alt=""/>
                {title && (
                    <div className="text-3xl font-medium text-center mt-12">{title}</div>
                )}
                {children}
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: {
        user: User
    }
}) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(AuthContainer)