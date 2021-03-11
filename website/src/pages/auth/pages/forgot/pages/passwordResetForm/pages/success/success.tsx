// npm imports
import React from "react"

// Function imports
import { routeMap, fractalRoute } from "shared/constants/routes"
import history from "shared/utils/history"

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"
import AuthButton from "pages/auth/shared/components/authButton"

const Success = () => {
    return (
        <AuthContainer title="Your password was reset successfully">
            <div className="text-gray mt-4 text-center">
                Please return to the login page and sign in with your new
                password.
            </div>
            <AuthButton text="Log in" onClick={() => history.push(fractalRoute(routeMap.AUTH))} />
        </AuthContainer>
    )
}

export default Success
