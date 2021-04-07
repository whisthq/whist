import React from 'react'

import { FractalFadeIn } from '@app/components/custom/fade'
import { Logo } from '@app/components/html/logo'
import { FractalInput, FractalInputState } from '@app/components/html/input'
import {
  FractalWarning,
  FractalWarningType
} from '@app/components/custom/warning'
import { FractalButton, FractalButtonState } from '@app/components/html/button'
import { FractalNavigation } from '@app/components/custom/navigation'

import {
  loginEnabled,
  checkEmail,
  checkPassword
} from '@app/utils/auth'

const Login = (props: {
  loading: boolean
  warning: string
  email: string
  password: string
  onLogin: () => void
  onNavigate: (s: string) => void
  onChangeEmail: (s: string) => void
  onChangePassword: (s: string) => void
}) => {
  /*
        Description:
            Component for logging in. Contains the login form UI and
            dispatches login API call

        Arguments:
            onLogin((json) => void):
                Callback function fired when login API call is sent, body of response
                is passed in as argument
    */

  const buttonState = () => {
    if (props.loading) { return FractalButtonState.PROCESSING }
    if (loginEnabled(props.email, props.password)) { return FractalButtonState.DEFAULT }
    return FractalButtonState.DISABLED
  }

  return (
        <div className="flex flex-col justify-center items-center h-screen text-center">
            <div className="w-full max-w-xs m-auto">
                <FractalFadeIn>
                    <Logo />
                    <h5 className="font-body mt-8 text-xl mb-6 font-semibold">
                        Log in to your account
                    </h5>
                    <FractalWarning
                        type={FractalWarningType.DEFAULT}
                        warning={props.warning}
                        className="mt-4"
                    />
                    <h5 className="font-body text-left font-semibold mt-7 text-sm">
                        Email
                    </h5>
                    <FractalInput
                        type="email"
                        placeholder="Email"
                        onChange={props.onChangeEmail}
                        onEnterKey={props.onLogin}
                        value={props.email}
                        state={
                            checkEmail(props.email)
                              ? FractalInputState.SUCCESS
                              : FractalInputState.DEFAULT
                        }
                        className="mt-1"
                    />
                    <h5 className="font-body text-left font-semibold mt-4 text-sm">
                        Password
                    </h5>
                    <FractalInput
                        type="password"
                        placeholder="Password"
                        onChange={props.onChangePassword}
                        onEnterKey={props.onLogin}
                        value={props.password}
                        state={
                            checkPassword(props.password)
                              ? FractalInputState.SUCCESS
                              : FractalInputState.DEFAULT
                        }
                        className="mt-1"
                    />
                    <FractalButton
                        contents="Log In"
                        className="mt-4 w-full"
                        state={buttonState()}
                        onClick={props.onLogin}
                    />
                    <FractalNavigation
                        url="/"
                        text="Need an account? Sign up here."
                        linkText="here"
                        className="relative top-4"
                        onClick={props.onNavigate}
                    />
                </FractalFadeIn>
            </div>
        </div>
  )
}

export default Login
