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
	graphql "github.com/hasura/go-graphql-client"
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
	stop := make(chan bool)
	server := subscription_setupServer()
	_, subscriptionClient := subscription_setupClients()
	msg := randomID()
	go func() {
		if err := server.ListenAndServe(); err != nil {
			log.Println(err)
		}
	}()

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer server.Shutdown(ctx)
	defer cancel()

	subscriptionClient.Hasura.
		OnError(func(sc *graphql.SubscriptionClient, err error) error {
			return err
		})

	/*
		subscription {
			helloSaid {
				id
				msg
			}
		}
	*/
	var sub struct {
		HelloSaid struct {
			ID      graphql.String
			Message graphql.String `graphql:"msg" json:"msg"`
		} `graphql:"helloSaid" json:"helloSaid"`
	}

	_, err := subscriptionClient.Hasura.Subscribe(sub, nil, func(data *json.RawMessage, e error) error {
		if e != nil {
			t.Fatalf("got error: %v, want: nil", e)
			return nil
		}

		log.Println("result", string(*data))
		e = json.Unmarshal(*data, &sub)
		if e != nil {
			t.Fatalf("got error: %v, want: nil", e)
			return nil
		}

		if sub.HelloSaid.Message != graphql.String(msg) {
			t.Fatalf("subscription message does not match. got: %s, want: %s", sub.HelloSaid.Message, msg)
		}

		t.Logf("Exiting of subscribe...")
		stop <- true

		return nil
	})

	if err != nil {
		t.Fatalf("got error: %v, want: nil", err)
	}

	go func() {
		err := subscriptionClient.Hasura.Run()
		if err != nil {
			log.Print(err)
		}
		stop <- true
	}()

	defer subscriptionClient.Close()

	// wait until the subscription client connects to the server
	time.Sleep(2 * time.Second)

	// call a mutation request to send message to the subscription
	/*
		mutation ($msg: String!) {
			sayHello(msg: $msg) {
				id
				msg
			}
		}
	*/
	// var q struct {
	// 	SayHello struct {
	// 		ID  graphql.String
	// 		Msg graphql.String
	// 	} `graphql:"sayHello(msg: $msg)"`
	// }
	// variables := map[string]interface{}{
	// 	"msg": graphql.String(msg),
	// }
	// t.Logf("Mutating to trigger sub")
	// err = client.Mutate(context.Background(), &q, variables, graphql.OperationName("SayHello"))
	// if err != nil {
	// 	t.Fatalf("got error: %v, want: nil", err)
	// }

	// t.Logf("1. Waiting on stop chan...")
	<-stop

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
	_, subscriptionClient := subscription_setupClients()

	ws, err := WhistWebsocketConn(subscriptionClient.Hasura)
	if err != nil {
		t.Fatalf("failed to create websocket connection: %s", err)
	}

	expectSocket, err := newTestConn("http://localhost:8080")
	if err != nil {
		t.Fatalf("failed to start test connection: %s", err)
	}

	ok := reflect.DeepEqual(expectSocket, ws)
	if !ok {
		t.Errorf("expected socket connection %v, got %v", expectSocket, ws)
	}
}
