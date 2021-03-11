// npm imports
import React from "react"

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"
import { PuffAnimation } from "shared/components/loadingAnimations"

const AuthLoader = (props: {
    title: string
}) => {
    const { title } = props
    return (
        <AuthContainer title={title}>
            <div>
                <PuffAnimation />
            </div>
        </AuthContainer>
    )
}

export default AuthLoader
