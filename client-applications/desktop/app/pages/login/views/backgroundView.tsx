import React from "react"

import styles from "styles/login.css"

import Blender from "assets/images/login/blender.svg"
import Chrome from "assets/images/login/chrome.svg"
import Figma from "assets/images/login/figma.svg"
import Maya from "assets/images/login/maya.svg"
import Photoshop from "assets/images/login/photoshop.svg"
import Slack from "assets/images/login/slack.svg"
import VSCode from "assets/images/login/vscode.svg"

const BackgroundView = (props: any) => {
    return (
        <div>
            <img
                src={Figma}
                className={styles.bounce}
                style={{
                    position: "absolute",
                    left: 10,
                    top: 80,
                    height: 130,
                    animationDelay: "0.0s",
                }}
            />
            <img
                src={Chrome}
                className={styles.bounce}
                style={{
                    position: "absolute",
                    left: 120,
                    top: 270,
                    height: 120,
                    animationDelay: "0.3s",
                }}
            />
            <img
                src={VSCode}
                className={styles.bounce}
                style={{
                    position: "absolute",
                    left: -10,
                    top: 500,
                    height: 120,
                    animationDelay: "0.6s",
                }}
            />
            <img
                src={Blender}
                className={styles.bounce}
                style={{
                    position: "absolute",
                    left: 820,
                    top: 100,
                    height: 115,
                    animationDelay: "0.9s",
                }}
            />
            <img
                src={Slack}
                className={styles.bounce}
                style={{
                    position: "absolute",
                    left: 720,
                    top: 220,
                    height: 110,
                    animationDelay: "1.2s",
                }}
            />
            <img
                src={Photoshop}
                className={styles.bounce}
                style={{
                    position: "absolute",
                    left: 800,
                    top: 420,
                    height: 110,
                    animationDelay: "1.5s",
                }}
            />
            <img
                src={Maya}
                className={styles.bounce}
                style={{
                    position: "absolute",
                    left: 730,
                    top: 560,
                    height: 140,
                    animationDelay: "1.8s",
                }}
            />
        </div>
    )
}

export default BackgroundView
