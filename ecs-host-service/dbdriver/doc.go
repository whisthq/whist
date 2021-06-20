// Package dbdriver contains the database-interfacing code for the host
// service.
//
// Here we explain some design decisions that went into choosing the tech stack
// for this particular package. Note that these decisions are not at all final,
// but I (@djsavvy) spent quite a while evaluating different technologies and
// therefore want to explain the reasoning behind the decisions that were
// ultimately made for v1 of this package.
//
// Q: Why use `pgx`?
// A: It is blessed by the official Go documentation and is not deprecated.
// It's also pretty well-documented, especially considering some of the other
// libraries I ran into.
//
// Q: Why not use `gorm`?
// A: I spent quite a while going through the documentation for `gorm`. It
// looks quite interesting. However, our existing database conventions are
// quite different from those recommened by `gorm`, which means that we would
// have to write quite a bit of boilerplate anyways. It is also slow, and
// everywhere online that I saw mentions of `gorm` I also saw complaints about
// straying off a very specific happy path of usage. I also noticed broken
// english and critical vaguenesses in the documentation that led me to believe
// using `gorm` would not actually save any time/effort compared to just
// writing close-to-raw SQL with `pgx`.
//
// Q: Why not use `sqlboiler`?
// A: The idea of generating code based on the schema caught my attention,
// since it would help us ensure that changes to the schema also get reflected
// in the host service code. However, the specific implementation proposed by
// `sqlboiler` isn't well-suited to our deployment since it would require us to
// spin up a local webserver deployment every time we wanted to build the host
// service. A solution that worked directly with `schema.sql` (which
// `sqlboiler` does not, as far as I can tell) would be ideal. Using
// `sqlboiler` would also require not to use views, which might be a
// dealbreaker down the line.
//
// Q: Why not use `xo`?
// A: This library is interesting for the same reason as `sqlboiler`. However,
// it expects us to use `pq` as the Postgres driver, which is deprecated.
//
// Q: Why use `pgx`'s interface instead of the standard `database/sql` one?
// A: Honestly I found `pgx`'s interface better-documented. The list of
// advantages at
// https://github.com/JackC/pgx#choosing-between-the-pgx-and-databasesql-interfaces
// also puts some weight on the `pgx` side of the scale.
//
package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"
