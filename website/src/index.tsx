import React from 'react'
import ReactDOM from 'react-dom'
import { Router } from 'react-router'
import history from '@app/shared/utils/history'
import RootApp from '@app/pages/root'

import '@app/styles/global.module.css'
import '@app/styles/bootstrap.css'
import { MainProvider } from '@app/shared/utils/context'

const RootComponent = () => (
        <Router history={history}>
            <MainProvider>
                <RootApp />
            </MainProvider>
        </Router>
)

ReactDOM.render(<RootComponent />, document.getElementById('root'))
