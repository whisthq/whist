import firebase from "firebase/app"
import "firebase/auth"
import "firebase/firestore"

import { config } from "constants/config"

var firebaseConfig =
    config.sentry_env === "development"
        ? {
              apiKey: process.env.REACT_APP_API_KEY_DEVELOPMENT,
              authDomain: process.env.REACT_APP_AUTH_DOMAIN_DEVELOPMENT,
              databaseURL: process.env.REACT_APP_DATABASE_URL_DEVELOPMENT,
              projectId: process.env.REACT_APP_PROJECT_ID_DEVELOPMENT,
              storageBucket: process.env.REACT_APP_STORAGE_BUCKET_DEVELOPMENT,
              messagingSenderId:
                  process.env.REACT_APP_MESSAGING_SENDER_ID_DEVELOPMENT,
          }
        : {
              apiKey: process.env.REACT_APP_API_KEY_PRODUCTION,
              authDomain: process.env.REACT_APP_AUTH_DOMAIN_PRODUCTION,
              databaseURL: process.env.REACT_APP_DATABASE_URL_PRODUCTION,
              projectId: process.env.REACT_APP_PROJECT_ID_PRODUCTION,
              storageBucket: process.env.REACT_APP_STORAGE_BUCKET_PRODUCTION,
              messagingSenderId:
                  process.env.REACT_APP_MESSAGING_SENDER_ID_PRODUCTION,
          }

firebase.initializeApp(firebaseConfig)

export const db = firebase.firestore()
