// npm imports
import React from "react"

// Function imports
import {routeMap, fractalRoute} from "shared/constants/routes"

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"
import AuthNavigator from "pages/auth/shared/components/authNavigator"

const Error = () => {
    return (
        <AuthContainer title="Password reset not allowed">
            <div className="text-gray mt-4 text-center">
                For security reasons, we are blocking your password reset request. If you believe this
                is a bug, please contact support@fractal.co for assistance.
            </div>
            <div className="mt-7">
                <AuthNavigator
                    beforeLink="Try again? Click "
                    link="here"
                    afterLink="."
                    redirect={fractalRoute(routeMap.AUTH.FORGOT)}
                />
            </div>
        </AuthContainer>
    )
}

export default Error
