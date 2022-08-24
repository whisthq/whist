package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	"context"
	"io"
	"net/url"
	"strings"
	"time"

	"github.com/gorilla/websocket"
	graphql "github.com/hasura/go-graphql-client"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// WhistWebsocketHandler implements `graphql.WebsocketHandler` and uses
// the `gorilla/websocket` library instead of the default used by Hasura.
type WhistWebsocketHandler struct {
	ctx     context.Context
	timeout time.Duration
	*websocket.Conn
}

// WriteJSON writes the JSON encoding of v as a message.
func (wh *WhistWebsocketHandler) WriteJSON(v interface{}) error {
	return wh.Conn.WriteJSON(v)
}

// ReadJSON reads the next JSON-encoded message from the connection and stores
// it in the value pointed to by v.
func (wh *WhistWebsocketHandler) ReadJSON(v interface{}) error {
	err := wh.Conn.ReadJSON(v)

	// This error always fires when shutting down the Hasura client because we close
	// the websocket concurrently. As it is not a harmful error we supress it here to
	// avoid clogging Sentry with it.
	// See: https://github.com/gorilla/websocket/issues/439
	if err != nil && strings.Contains(err.Error(), "use of closed network connection") {
		return nil
	}

	// Since version 0.7.2, The hasura graphql triggers a segfault when receiving an
	// EOF error. Additionally, an EOF error will silently terminate the subscription.
	// This condition logs the error as a warning instead (because it checks for == "EOF").
	// and returns a nil error. TODO: remove once the segfault is resolved.
	if err != nil && (err == io.ErrUnexpectedEOF || err == io.EOF || strings.Contains(err.Error(), "EOF")) {
		logger.Warningf("unexpected end of input")
		return nil
	}

	return err
}

// Close closes the underlying network connection without sending or waiting
// for a close message.
func (wh *WhistWebsocketHandler) Close() error {
	return wh.Conn.Close()
}

// WhistWebsocketConn creates a websocket handler and passes it to the GraphQL
// subscription client.
func WhistWebsocketConn(sc *graphql.SubscriptionClient) (graphql.WebsocketConn, error) {
	parsedURL, err := url.Parse(sc.GetURL())
	if err != nil {
		return nil, err
	}

	u := url.URL{Scheme: "ws", Host: parsedURL.Host, Path: parsedURL.Path}
	c, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
	if err != nil {
		return nil, err
	}

	return &WhistWebsocketHandler{
		ctx:     sc.GetContext(),
		Conn:    c,
		timeout: sc.GetTimeout(),
	}, nil
}
