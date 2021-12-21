package auth

import (
	"encoding/json"
	"reflect"
	"testing"

	"github.com/golang-jwt/jwt/v4"
)

func TestUnmarshalScopes(t *testing.T) {
	// Ensure that UnmarshalJSON returns an error when given invalid data
	t.Run("InvalidJSON", func(t *testing.T) {
		t.Parallel()

		scope := make(Scopes, 0)
		err := scope.UnmarshalJSON([]byte{})

		if err == nil {
			t.Fatal("UmarshalJSON should have an err")
		}
	})

	// Ensure that UnmarshalJSON unmarshals an empty scopes type when passed an
	// empty byte slice.
	t.Run("EmptyString", func(t *testing.T) {
		t.Parallel()

		want := Scopes{}
		got := make(Scopes, 0)
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

		want := Scopes{"hello"}
		got := make(Scopes, 1)
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

		want := Scopes{"hello", "world"}
		got := make(Scopes, 2)
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

		want := Scopes{"hello"}
		got := Scopes{"hi"}
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

		want := Scopes{"hello", "world"}
		got := make(Scopes, 2)
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
			Scopes Scopes `json:"scope"`
		}

		want := myStruct{Scopes{"hello", "world"}}
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


func TestUnmarshalAudience(t *testing.T) {
	// Ensure that UnmarshalJSON returns an error when it cannot unmarshal to string
	t.Run("InvalidJSON", func(t *testing.T) {
		t.Parallel()

		audience := make(Audience, 0)
		// UnmarshalJSON will throw an error as the byte array does not contain a string/string splice
		err := audience.UnmarshalJSON([]byte(`{"test":"not string"}`))

		if _, ok := err.(*json.UnmarshalTypeError); !ok || err == nil {
			t.Fatal("UmarshalJSON should have an err")
		}
	})
}

func TestParseToken(t *testing.T) {
	// Ensure an invalid token will result in an error
	t.Run("InvalidToken", func(t *testing.T) {
		t.Parallel()

		token := "not_a_real_token"
		// ParseToken should return an error as the token is invalid
		claim, err := ParseToken(token)

		if err == nil {
			t.Fatal("ParseToken should have returned an err")
		}

		if claim != nil {
			t.Fatalf("ParseToken should have set claim to nil but got %v", claim)
		}
	})
}

func TestVerify(t *testing.T) {
	// Ensure nil claims will result in an error
	t.Run("NilClaim", func(t *testing.T) {
		t.Parallel()

		// Verify should return an error as the claims is nil
		err := Verify(nil)

		if err == nil {
			t.Fatal("Verify should have returned an err")
		}
	})

	// Ensure claims without aud will result in an error
	t.Run("ClaimsWithoutAud", func(t *testing.T) {
		t.Parallel()

		// token is an H256-signed JWT whose payload is {}
		token := "eyJraWQiOiI1eTV0SUxqcUVvT0tyT0t6RGJTMXgiLCJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCIsImhtYWMiOiJubyJ9.e30.piTMYmm7MvoJ8dAXSwpJhZJStB90TgCcHgy0-uLja1g"

		claims := new(WhistClaims)
		parser := new(jwt.Parser)
		_, _, err := parser.ParseUnverified(token, claims)

		if err != nil {
			t.Fatal(err)
		}

		// Verify should return an error as the claims will not match the config's audience
		err = Verify(claims)

		if err == nil {
			t.Fatal("ParseToken should have returned an err")
		}
	})

	// Ensure claims with missing Iss will result in an error
	t.Run("ClaimsWIthoutIss", func(t *testing.T) {
		t.Parallel()

		// token is an H256-signed JWT whose payload is {"aud": "https://api.fractal.co"}
		token := "eyJraWQiOiI1eTV0SUxqcUVvT0tyT0t6RGJTMXgiLCJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCIsImhtYWMiOiJubyJ9.eyJhdWQiOiJodHRwczovL2FwaS5mcmFjdGFsLmNvIn0.1UkhyuDTJ59e6ZSFgVIgGGj1Xo59XxLoeFpNyx9ssr8"
		// Verify should return an error as the claims are missing Iss
		claims := new(WhistClaims)
		parser := new(jwt.Parser)
		_, _, err := parser.ParseUnverified(token, claims)

		if err != nil {
			t.Fatal(err)
		}

		err = Verify(claims)

		if err == nil {
			t.Fatal("Verify should have returned an err because Iss is missing")
		}
	})

	// Ensure a valid claims will pass all verifies
	t.Run("ValidClaims", func(t *testing.T) {

		// token is an H256-signed JWT whose payload matches config dev
		token := "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJodHRwczovL2FwaS5mcmFjdGFsLmNvIiwiaXNzIjoiaHR0cHM6Ly9mcmFjdGFsLWRldi51cy5hdXRoMC5jb20vIn0.kYmU7bGXvpRuISydcIYju2yoUy018vkPD6y1tcvxzVM"
		// Verify should return nil as the claims are valid
		claims := new(WhistClaims)
		parser := new(jwt.Parser)
		_, _, err := parser.ParseUnverified(token, claims)

		if err != nil {
			t.Fatal(err)
		}

		err = Verify(claims)

		if err != nil {
			t.Fatalf("Verify should have returned nil but got %v", err)
		}	
	})
}

func TestVerifyAudience(t *testing.T) {
	// Ensure that tokens whose "aud" claim is a string are parsed correctly.
	t.Run("OneAudience", func(t *testing.T) {
		t.Parallel()

		// rawToken is an H256-signed JWT whose payload is {"aud": "foo"}
		const rawToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJmb28ifQ.h44sAymzUhQYXbdOqrCkr7aEf0HkU-wbgWQdQ74Q288"

		claims := new(WhistClaims)
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

		claims := new(WhistClaims)
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

		claims := new(WhistClaims)

		if claims.VerifyScope("foo") {
			t.Fatal("VerifyScope should return false")
		}
	})

	// Ensure that VerifyScope returns false when the requested scope is not
	// present.
	t.Run("Mismatch", func(t *testing.T) {
		t.Parallel()

		claims := &WhistClaims{
			Scopes: Scopes{"foo", "bar"},
		}

		if claims.VerifyScope("baz") {
			t.Fatal("VerifyScope should return false")
		}
	})

	// Ensure that VerifyScope returns true when the requested scope is present.
	t.Run("Match", func(t *testing.T) {
		t.Parallel()

		claims := &WhistClaims{
			Scopes: Scopes{"foo", "bar", "baz"},
		}

		if !claims.VerifyScope("bar") {
			t.Fatal("VerifyScope should return true")
		}
	})
}

