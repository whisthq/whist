# Module Structure

At the module level, the code is separated between AWS, mail, admin, payment, auth, and celery blueprints (post and get request handlers)

## Admin

This folder contains our admin login, logs, report generation, and admin dashboard endpoints.

## Auth

This folder contains our account generation and manipulation endpoints, plus login.

## AWS

This folder contains our AWS container creation, deletion, modification and ping endpoints

## Mail

This folder contains our email generation and send endpoints

## Payment

This folder contains our stripe handling/payment endpoints

## Celery

This folder contains our celery task status getter endpoints.
