import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import TypeWriterEffect from 'react-typewriter-effect';


import { db } from "utils/firebase";

import "styles/landing.css";

import WaitlistForm from "pages/landing/components/waitlistForm";
import CountdownTimer from "pages/landing/components/countdown";

import LaptopAwe from "assets/largeGraphics/laptopAwe.svg";
import Moon from "assets/largeGraphics/moon.svg";
import Mars from "assets/largeGraphics/mars.svg";
import Mercury from "assets/largeGraphics/mercury.svg";
import Saturn from "assets/largeGraphics/saturn.svg";
import Plants from "assets/largeGraphics/plants.svg";

function Landing(props: any) {
  const [state, setState] = useState(() => {
    return ({
      "waitlist": []
    })
  })

  useEffect(() => {
    getWaitlist().then(function (waitlist) {
      setState(prevState => { return { ...prevState, "waitlist": waitlist } })
    })
  }, []);

  async function getWaitlist() {
    const waitlist = await db.collection("waitlist").get()
    return (waitlist.docs.map(doc => doc.data()))
  }

  return (
    <div>
      <div className="banner-background" style={{ zIndex: 2, width: "100vw" }}>
        <img src={Moon} style={{ position: "absolute", width: 100, height: 100, top: 125, left: 40 }} />
        <img src={Mars} style={{ position: "absolute", width: 120, height: 120, top: 185, right: -40 }} />
        <img src={Mercury} style={{ position: "absolute", width: 80, height: 80, top: 405, right: 90 }} />
        <img src={Saturn} style={{ position: "absolute", width: 100, height: 100, top: 425, left: 80 }} />
        <div style={{ display: "flex", justifyContent: "space-between", width: "100%", padding: 30 }}>
          <div className="logo">
            Fractal
          </div>
          <div style={{ position: "relative", right: 100 }}>
            <CountdownTimer />
          </div>
        </div>
        <div style={{ margin: "auto", marginTop: 20, marginBottom: 20, color: "white", textAlign: "center", width: 800, }}>
          <div style={{ display: "flex", margin: "auto", justifyContent: "center" }}>
            <TypeWriterEffect
              textStyle={{ fontFamily: "Maven Pro", color: "#00D4FF", fontSize: 70, fontWeight: "bold" }}
              startDelay={0}
              cursorColor="white"
              multiText={["Blender", "Figma", "Adobe", "Maya", "Blender"]}
              typeSpeed={150}
            />
            <div style={{ fontWeight: "bold", fontSize: 70, paddingBottom: 40 }}>
              , just <span style={{ color: "#00D4FF" }}>faster</span>.
          </div>
          </div>
          <div style={{ fontSize: 15, width: 600, margin: "auto", lineHeight: 1.5 }}>
            Fractal uses cloud streaming to supercharge your laptop's applications. <br /> To get access, join our waitlist before the countdown ends.
          </div>
          <div style={{ marginTop: 50 }}>
            <WaitlistForm />
          </div>
        </div>
        <div style={{ margin: "auto", maxWidth: 600, position: "relative", bottom: 60, zIndex: 2 }}>
          <img src={LaptopAwe} style={{ maxWidth: 600 }} />
        </div>
      </div>
      <div className="white-background-curved" style={{ height: 350, position: "relative", bottom: 150, background: "white", zIndex: 1 }}>
        <img src={Plants} style={{ position: "absolute", width: 250, left: 0, top: 10 }} />
        <img src={Plants} style={{ position: "absolute", width: 250, right: 0, top: 10, transform: "scaleX(-1)" }} />
      </div>
    </div>
  );
}

function mapStateToProps(state) {
  return {
    user: state.AuthReducer.user
  }
}

export default connect(mapStateToProps)(Landing);
