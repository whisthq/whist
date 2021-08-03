package auth_test

import (
	"encoding/json"
	"reflect"
	"testing"

	"github.com/fractal/fractal/host-service/auth"

	"github.com/golang-jwt/jwt"
)

func TestUnmarshalScopes(t *testing.T) {
	// Ensure that UnmarshalJSON unmarshals an empty scopes type when passed an
	// empty byte slice.
	t.Run("EmptyString", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{}
		got := make(auth.Scopes, 0)
		err := got.UnmarshalJSON([]byte(`""`))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalJSON should unmarshal %v", want)
		}
	})

	// Ensure that UnmarshalJSON unmarshals a scopes type containing a single
	// scope when passed a byte slice that does not contain ' '.
	t.Run("Singleton", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{"hello"}
		got := make(auth.Scopes, 1)
		err := got.UnmarshalJSON([]byte(`"hello"`))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalJSON should unmarshal %v", want)
		}
	})

	// Ensure that UnmarshalJSON unmarshals a scopes type containing two scopes
	// when passed a byte slice containing a single ' '.
	t.Run("Multi", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{"hello", "world"}
		got := make(auth.Scopes, 2)
		err := got.UnmarshalJSON([]byte(`"hello world"`))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalJSON should unmarshal %v", want)
		}
	})

	// Ensure that UnmarshalJSON overwrites any data that is already present in
	// the scopes type on which it is called.
	t.Run("Overwrite", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{"hello"}
		got := auth.Scopes{"hi"}
		err := got.UnmarshalJSON([]byte(`"hello"`))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalJSON should unmarshal %v", want)
		}
	})

	// Ensure that json.Unmarshal calls UnmarshalJSON.
	t.Run("UnmarshalValue", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{"hello", "world"}
		got := make(auth.Scopes, 2)
		err := json.Unmarshal([]byte(`"hello world"`), &got)

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("json.Unmarshal should unmarshal %v", want)
		}
	})

	// Ensure that json.Unmarshal calls UnmarshalJSON.
	t.Run("UnmarshalStruct", func(t *testing.T) {
		t.Parallel()

		type myStruct struct {
			Scopes auth.Scopes `json:"scope"`
		}

		want := myStruct{auth.Scopes{"hello", "world"}}
		got := myStruct{}
		err := json.Unmarshal([]byte(`{"scope": "hello world"}`), &got)

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("json.Unmarshal should unmarshal %+v", want)
		}
	})
}

func TestParseToken(t *testing.T) {
	// Ensure that tokens whose "aud" claim is a string are parsed correctly.
	t.Run("OneAudience", func(t *testing.T) {
		t.Parallel()

		// rawToken is an H256-signed JWT whose payload is {"aud": "foo"}
		const rawToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJmb28ifQ.h44sAymzUhQYXbdOqrCkr7aEf0HkU-wbgWQdQ74Q288"

		claims := new(auth.FractalClaims)
		parser := new(jwt.Parser)
		_, _, err := parser.ParseUnverified(rawToken, claims)

		if err != nil {
			t.Fatal(err)
		}

		if !claims.VerifyAudience("foo", true) {
			t.Fatal("VerifyAudience should return true")
		}
	})

	// Ensure that tokens whose "aud" claim is a list of strings are parsed
	// correctly.
	t.Run("MultiAudience", func(t *testing.T) {
		t.Parallel()

		// rawToken is an H256-signed JWT whose payload is
		// {"aud": ["hello", "world"]}.
		const rawToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsiaGVsbG8iLCJ3b3JsZCJdfQ.ckOGyW7IkarMXd8-KxlDXB9LbdFwN06tVqraRXjRRyk"

		claims := new(auth.FractalClaims)
		parser := new(jwt.Parser)
		_, _, err := parser.ParseUnverified(rawToken, claims)

		if err != nil {
			t.Fatal(err)
		}

		if !claims.VerifyAudience("hello", true) {
			t.Fatal("VerifyAudience should return true")
		}
	})
}

func TestVerifyScope(t *testing.T) {
	// Ensure that VerifyScope returns false when no scopes are present.
	t.Run("Empty", func(t *testing.T) {
		t.Parallel()

		claims := new(auth.FractalClaims)

		if claims.VerifyScope("foo") {
			t.Fatal("VerifyScope should return false")
		}
	})

	// Ensure that VerifyScope returns false when the requested scope is not
	// present.
	t.Run("Mismatch", func(t *testing.T) {
		t.Parallel()

		claims := &auth.FractalClaims{
			Scopes: auth.Scopes{"foo", "bar"},
		}

		if claims.VerifyScope("baz") {
			t.Fatal("VerifyScope should return false")
		}
	})

	// Ensure that VerifyScope returns true when the requested scope is present.
	t.Run("Match", func(t *testing.T) {
		t.Parallel()

		claims := &auth.FractalClaims{
			Scopes: auth.Scopes{"foo", "bar", "baz"},
		}

		if !claims.VerifyScope("bar") {
			t.Fatal("VerifyScope should return true")
		}
	})
}
