package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	"context"
	"net/url"
	"time"

	"github.com/gorilla/websocket"
	graphql "github.com/hasura/go-graphql-client"
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
	return wh.Conn.ReadJSON(v)
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
