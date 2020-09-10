import React, { useState, useEffect } from "react";
import { connect } from "react-redux";

import { db } from "utils/firebase";

import "styles/landing.css";

import WaitlistForm from "pages/landing/components/waitlistForm";
import CountdownTimer from "pages/landing/components/countdown";

import LaptopAwe from "assets/largeGraphics/laptopAwe.svg";
import Moon from "assets/largeGraphics/moon.svg";
import Mars from "assets/largeGraphics/mars.svg";
import Mercury from "assets/largeGraphics/mercury.svg";
import Saturn from "assets/largeGraphics/saturn.svg";

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
      <div className="banner-background" style={{ zIndex: 2 }}>
        <img src={Moon} style={{ position: "absolute", width: 100, height: 100, top: 125, left: 40 }} />
        <img src={Mars} style={{ position: "absolute", width: 120, height: 120, top: 185, right: -40 }} />
        <img src={Mercury} style={{ position: "absolute", width: 80, height: 80, top: 405, right: 90 }} />
        <img src={Saturn} style={{ position: "absolute", width: 100, height: 100, top: 425, left: 80 }} />
        <div style={{ display: "flex", justifyContent: "space-between", width: "100%", padding: 30 }}>
          <div className="logo">
            Fractal
          </div>
        </div>
        <div style={{ margin: "50px auto", color: "white", textAlign: "center", width: 800, position: "relative", bottom: 75 }}>
          <div style={{ marginBottom: 50 }}>
            <CountdownTimer />
          </div>
          <div style={{ fontWeight: "bold", fontSize: 75, paddingBottom: 40 }}>
            <span style={{ color: "#00D4FF" }}>Blender</span> on your computer, just <span style={{ color: "#00D4FF" }}>faster</span>.
          </div>
          <div style={{ fontSize: 16, marginLeft: 100, marginRight: 100 }}>
            Fractal uses cloud streaming to run creative apps on your laptop without slowing down.
          </div>
        </div>
        <div style={{ margin: "auto", maxWidth: 600, position: "relative", bottom: 50, zIndex: 2 }}>
          <img src={LaptopAwe} style={{ maxWidth: 600 }} />
        </div>
      </div>
      <div className="white-background-curved" style={{ height: 350, position: "relative", bottom: 150, background: "white", zIndex: 1 }}>

      </div>
      <WaitlistForm />
    </div>
  );
}

function mapStateToProps(state) {
  return {
    user: state.AuthReducer.user
  }
}

export default connect(mapStateToProps)(Landing);
