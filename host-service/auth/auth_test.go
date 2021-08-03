package auth_test

import (
	"encoding/json"
	"reflect"
	"testing"

	"github.com/fractal/fractal/host-service/auth"
)

func TestUnmarshalScopes(t *testing.T) {
	// Ensure that UnmarshalText returns an error when called on a nil scopes
	// pointer.
	t.Run("NilPointer", func(t *testing.T) {
		t.Parallel()

		nilPtr := (*auth.Scopes)(nil)
		err := nilPtr.UnmarshalText([]byte(""))

		if err == nil {
			t.Fatal("UnmarshalText should return an error")
		}
	})

	// Ensure that UnmarshalText unmarshals an empty scopes type when passed an
	// empty byte slice.
	t.Run("EmptyString", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{}
		got := make(auth.Scopes, 0)
		err := got.UnmarshalText([]byte(""))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalText should unmarshal %v", want)
		}
	})

	// Ensure that UnmarshalText unmarshals a scopes type containing a single
	// scope when passed a byte slice that does not contain ' '.
	t.Run("Singleton", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{"hello"}
		got := make(auth.Scopes, 1)
		err := got.UnmarshalText([]byte("hello"))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalText should unmarshal %v", want)
		}
	})

	// Ensure that UnmarshalText unmarshals a scopes type containing two scopes
	// when passed a byte slice containing a single ' '.
	t.Run("Multi", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{"hello", "world"}
		got := make(auth.Scopes, 2)
		err := got.UnmarshalText([]byte("hello world"))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalText should unmarshal %v", want)
		}
	})

	// Ensure that UnmarshalText overwrites any data that is already present in
	// the scopes type on which it is called.
	t.Run("Overwrite", func(t *testing.T) {
		t.Parallel()

		want := auth.Scopes{"hello"}
		got := auth.Scopes{"hi"}
		err := got.UnmarshalText([]byte("hello"))

		if err != nil {
			t.Fatal(err)
		}

		if !reflect.DeepEqual(got, want) {
			t.Fatalf("UnmarshalText should unmarshal %v", want)
		}
	})

	// Ensure that UnmarshalText unmarshals JSON strings correctly.
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

	// Ensure that UnmarshalText unmarshals JSON strings correctly.
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
