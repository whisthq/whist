package subscriptions

import (
	"context"
	"log"
	"net/http"
	"net/http/httptest"
	"net/url"
	"reflect"
	"testing"
	"time"

	"github.com/gorilla/websocket"
)

func socketHandler(w http.ResponseWriter, r *http.Request) {
	upgrader := websocket.Upgrader{}
	c, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		return
	}
	defer c.Close()
	for {
		mt, message, err := c.ReadMessage()
		if err != nil {
			log.Println("read:", err)
			break
		}
		log.Printf("recv message %v", message)
		err = c.WriteMessage(mt, message)
		if err != nil {
			log.Println("write:", err)
			break
		}
	}
}

func TestJSON(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(socketHandler))
	defer srv.Close()

	parsedURL, err := url.Parse(srv.URL)
	if err != nil {
		t.Fatalf("failed to parse test server url: %s", err)
	}

	u := url.URL{Scheme: "ws", Host: parsedURL.Host, Path: parsedURL.Path}
	c, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
	if err != nil {
		t.Fatalf("failed to create mock websocket connection: %s", err)
	}

	mockSocket := &WhistWebsocketHandler{
		ctx:     context.Background(),
		Conn:    c,
		timeout: time.Second * 1,
	}

	var got, expect struct {
		testInt    int
		testString string
		testBool   bool
	}

	expect.testInt = 9
	expect.testString = "test_whist_socket_string"
	expect.testBool = true

	err = mockSocket.WriteJSON(&expect)
	if err != nil {
		t.Fatalf("failed to write JSON: %s", err)
	}

	err = mockSocket.ReadJSON(&got)
	if err != nil {
		t.Fatalf("failed to read JSON: %s", err)
	}

	ok := reflect.DeepEqual(expect, got)
	if !ok {
		t.Errorf("expected %v, got %v", expect, got)
	}

}

func TestClose(t *testing.T) {

}

func TestWhistWebsocketConn(t *testing.T) {

}
