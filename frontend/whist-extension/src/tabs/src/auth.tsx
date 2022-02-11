import React from "react"

const LoginButton = (props: { login: () => void }) => (
  <button onClick={props.login}>Log in</button>
)

const LoginTitle = () => <div>Welcome to Whist</div>

const LoginSubtitle = () => (
  <div>To unlock cloud tabs, please sign in or create an account</div>
)

const LoginComponent = () => {
  return (
    <div>
      <LoginTitle />
      <LoginSubtitle />
      <LoginButton login={() => {}} />
    </div>
  )
}

export default () => (
  <>
    <LoginComponent />
  </>
)
