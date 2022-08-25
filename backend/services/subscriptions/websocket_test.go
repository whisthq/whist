package subscriptions

import (
	"context"
	"encoding/json"
	"log"
	"net/http"
	"net/http/httptest"
	"net/url"
	"reflect"
	"testing"
	"time"

	"github.com/gorilla/websocket"
	"github.com/whisthq/whist/backend/services/utils"
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
		log.Printf("recv message %s", message)
		err = c.WriteMessage(mt, message)
		if err != nil {
			log.Println("write:", err)
			break
		}
	}
}

func newTestConn(serverURL string) (*WhistWebsocketHandler, error) {
	parsedURL, err := url.Parse(serverURL)
	if err != nil {
		return nil, utils.MakeError("failed to parse test server url: %s", err)
	}

	u := url.URL{Scheme: "ws", Host: parsedURL.Host, Path: parsedURL.Path}
	c, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
	if err != nil {
		return nil, utils.MakeError("failed to create mock websocket connection: %s", err)
	}

	return &WhistWebsocketHandler{
		ctx:     context.Background(),
		Conn:    c,
		timeout: time.Second * 1,
	}, nil

}

func TestJSON(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(socketHandler))
	defer srv.Close()

	testSocket, err := newTestConn(srv.URL)
	if err != nil {
		t.Fatalf("failed to create test connection: %s", err)
	}

	jsonString := `{"test_int":9,"test_string":"test_whist_socket_string","test_bool":true}`

	var got, expect struct {
		TestInt    int32  `json:"test_int"`
		TestString string `json:"test_string"`
		TestBool   bool   `json:"test_bool"`
	}

	expect.TestInt = 9
	expect.TestString = "test_whist_socket_string"
	expect.TestBool = true

	err = testSocket.WriteJSON(jsonString)
	if err != nil {
		t.Fatalf("failed to write JSON: %s", err)
	}

	var res string
	err = testSocket.ReadJSON(&res)
	if err != nil {
		t.Fatalf("failed to read JSON: %s", err)
	}

	// Since ReadJSON returns a JSON string, unmarshal
	// and compare with the expected value.
	err = json.Unmarshal([]byte(res), &got)
	if err != nil {
		t.Fatalf("failed to unmarshal readJSON result: %s", err)
	}

	ok := reflect.DeepEqual(expect, got)
	if !ok {
		t.Errorf("expected %v, got %v", expect, got)
	}
}

func TestEOF(t *testing.T) {
	// srv := httptest.NewServer(http.HandlerFunc(eofHandler))
	// defer srv.Close()

	// testClient := &SubscriptionClient{}
	// testClient.SetParams(HasuraParams{
	// 	URL:       srv.URL,
	// 	AccessKey: "hasura",
	// })
	// testClient.Initialize(false)
}

func TestClose(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(socketHandler))
	defer srv.Close()

	testSocket, err := newTestConn(srv.URL)
	if err != nil {
		t.Fatalf("failed to start test connection: %s", err)
	}

	err = testSocket.Close()
	if err != nil {
		t.Fatalf("got an error when closing connection: %s", err)
	}

	err = testSocket.WriteJSON("test")
	if err == nil {
		t.Errorf("expected error after closing connection, received nil error")
	}
}

func TestWhistWebsocketConn(t *testing.T) {
	// srv := httptest.NewServer(http.HandlerFunc(socketHandler))
	// defer srv.Close()

	// mockSubscriptionClient := &mockWhistClient{}
	// mockSubscriptionClient.SetParams(HasuraParams{
	// 	URL:       srv.URL,
	// 	AccessKey: "hasura",
	// })

	// expectSocket, err := newTestConn(srv.URL)
	// if err != nil {
	// 	t.Fatalf("failed to start test connection: %s", err)
	// }

	// gotSocket, err := WhistWebsocketConn(mockSubscriptionClient)
	// if err != nil {
	// 	t.Errorf("got error while creating a socket handler: %s", err)
	// }

	// ok := reflect.DeepEqual(expectSocket, gotSocket)
	// if !ok {
	// 	t.Errorf("expected connection to be %v, got %v", expectSocket, gotSocket)
	// }

}
