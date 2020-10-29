import React, { CSSProperties } from "react"

import { BsGraphUp, BsWrench, BsFillGearFill} from "react-icons/bs"
import DivSpace from "shared/components/divSpace"
import Header from "shared/components/header"

import "styles/shared.css"

const iconStyle: CSSProperties = {
    color: "black",
    position: "relative",
    top: 5,
    marginRight: 15,
}

const Graph = <BsGraphUp style={iconStyle} />
const Wrench = <BsWrench style={iconStyle} />
const Gear = <BsFillGearFill style={iconStyle} />

const contactUsLink = <a
    href="mailto: careers@fractalcomputers.com"
    className="header-link"
>
    Careers
</a>

const Career = (props: {
    title: string
    subtitle: string
    description: string
    icon: any
}) => {
    const { title, subtitle, icon} = props


    return (
        // CODE REUSE!??!?
        <div>
            <div
                style={{
                    color: "#111111",
                    fontSize: 24,
                    fontWeight: "bold",
                    textAlign: "center",
                }}
            >
                {title}
            </div>
            <div
                style={{
                    color: "#111111",
                    fontSize: 16,
                    fontWeight: "bold",
                    textAlign: "center",
                }}
            >
                {subtitle}
            </div>
            {icon}
        </div>
    )
}

const Careers = (props: any) => {
    return (
        <div className="fractalContainer">
            <Header color="black" />
            <div
                style={{
                    width: 400,
                    margin: "auto",
                    marginTop: 70,
                }}
            >
                <div
                    style={{
                        color: "#111111",
                        fontSize: 32,
                        fontWeight: "bold",
                        textAlign: "center",
                    }}
                >
                    Positions Available
                </div>
                <DivSpace height={40} />
                <Career title="Fullstack Engineer" subtitle="Internship or Fulltime" description="" icon={Wrench}/>
                <DivSpace height={40} />
                <Career title="Systems Engineer" subtitle="Internship or Fulltime" description="" icon={Gear}/>
                <DivSpace height={40} />
                <Career title="Product Marketing" subtitle="Internship" description=""  icon={Graph}/>
            </div>
        </div>
    )
}

export default Careers
