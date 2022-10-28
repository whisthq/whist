FROM golang:1.19-alpine3.16 as build

WORKDIR /app

ARG GIT_COMMIT
ARG HEROKU_USER
ARG HEROKU_API_KEY
ARG HEROKU_APP_NAME_OVERRIDE

# pre-copy/cache go.mod for pre-downloading dependencies and only redownloading them in subsequent builds if they change
COPY go.mod go.sum ./
RUN go mod download && go mod verify

COPY . .

RUN go build -ldflags "\
		-X 'github.com/whisthq/whist/backend/services/metadata.gitCommit=$GIT_COMMIT'\
		-X 'github.com/whisthq/whist/backend/services/metadata/heroku.email=$HEROKU_USER'\
		-X 'github.com/whisthq/whist/backend/services/metadata/heroku.apiKey=$HEROKU_API_KEY'\
		-X 'github.com/whisthq/whist/backend/services/metadata/heroku.appNameOverride=$HEROKU_APP_NAME_OVERRIDE'\
    " -v -o ./build/scaling-service ./scaling-service

# Create another stage to avoid copying the source code
FROM alpine:3.16

COPY --from=build /app/build/scaling-service /usr/local/bin/scaling-service

RUN mkdir -p /etc/whist
COPY --from=build /app/images.json /etc/whist/images.json

CMD ["scaling-service"]
