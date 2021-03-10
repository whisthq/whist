// npm imports
import React from "react"

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"
import { PuffAnimation } from "shared/components/loadingAnimations"

const Error = () => {
    return (
        <AuthContainer title="Please wait while we authenticate you">
            <div>
                <PuffAnimation />
            </div>
        </AuthContainer>
    )
}

export default Error
