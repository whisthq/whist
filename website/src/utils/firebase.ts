import * as firebase from "firebase";

var firebaseConfig = {
    apiKey: "AIzaSyBUBhvhoqZDPNZeHmX7Bx7PU6ukfpiRszE",
    authDomain: "new-website-staging.firebaseapp.com",
    databaseURL: "https://new-website-staging.firebaseio.com",
    projectId: "new-website-staging",
    storageBucket: "new-website-staging.appspot.com",
    messagingSenderId: "772828565876",
    appId: "1:772828565876:web:970e051371b640d55bdc21",
    measurementId: "G-V3N1J8WKW5",
};

firebase.initializeApp(firebaseConfig);

export const db = firebase.firestore();
