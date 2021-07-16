import React, { useState, ReactNode } from "react"
import { withClass } from "@app/shared/utils/withClass"
import history from "@app/shared/utils/history"
import classNames from "classnames"
import {
  AboutLink,
  SupportLink,
  CareersLink,
  LogoLink,
  WordmarkLink,
  HomeLink,
} from "@app/shared/components/links"
import {
  JustifyStartEndRow,
  JustifyStartEndCol,
  ScreenFull,
} from "@app/shared/components/layouts"

const mobileHidden = "hidden md:inline"

const Logo = (props: { className?: string; dark: boolean }) => (
  <div
    className={classNames(
      "flex items-center translate-x-1 space-x-6 mr-10",
      props.className
    )}
  >
    <LogoLink className="w-6 transform md:-translate-y-0.5" dark={props.dark} />
    <WordmarkLink
      className={classNames(
        "text-xl font-medium text-gray dark:text-gray-100 hover:text-gray dark:hover:text-white",
        mobileHidden
      )}
    />
  </div>
)

const startButtonClasses = classNames(
  "text-gray-500 dark:text-gray-300 hover:text-blue dark:hover:text-mint",
  "duration-500 font-body tracking-widest hover:no-underline whitespace-nowrap"
)

const AboutLinkStyled = withClass(AboutLink, startButtonClasses)
const SupportLinkStyled = withClass(SupportLink, startButtonClasses)
const CareersLinkStyled = withClass(CareersLink, startButtonClasses)
const HomeLinkStyled = withClass(HomeLink, startButtonClasses)

const StartHeaderRow = (props: { children?: ReactNode[] }) => (
  <div className="flex items-center space-x-6">{props.children}</div>
)

const StartHeaderCol = (props: {
  className?: string
  children?: ReactNode[] | ReactNode
}) => (
  <div className={classNames("flex flex-col items-center space-y-2")}>
    {props.children}
  </div>
)

const Header = (props: {
  onAccountPage?: boolean | false
  isSignedIn?: boolean | false
  dark: boolean
}) => {
  const [expanded, setExpanded] = useState(false)
  history.listen(() => setExpanded(false))

  return (
    <div>
      <div className="relative w-full">
        <div className="flex w-full mt-4 md:h-12 items-center">
          <JustifyStartEndRow
            start={
              <StartHeaderRow>
                <Logo dark={props.dark} />
                {props.onAccountPage === undefined && (
                  <>
                    <AboutLinkStyled className={mobileHidden} />
                    <SupportLinkStyled className={mobileHidden} />
                    <CareersLinkStyled className={mobileHidden} />
                  </>
                )}
              </StartHeaderRow>
            }
            end={<></>}
          />
        </div>
        <ScreenFull
          className={classNames(
            "md:hidden",
            props.dark !== undefined ? "bg-blue-darkest" : "bg-white",
            !expanded && "hidden"
          )}
        >
          <div className="flex w-full mt-36">
            <JustifyStartEndCol
              start={
                <StartHeaderCol>
                  {(props.isSignedIn ?? false) &&
                  (props.onAccountPage ?? false) ? (
                    <>
                      <HomeLinkStyled />
                    </>
                  ) : (
                    <>
                      <AboutLinkStyled />
                      <SupportLinkStyled />
                      <CareersLinkStyled />
                    </>
                  )}
                </StartHeaderCol>
              }
              middle={<div className="h-8"></div>}
              end={<></>}
            />
          </div>
        </ScreenFull>
      </div>
    </div>
  )
}

export default Header
