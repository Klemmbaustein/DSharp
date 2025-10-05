# D# scripting language

D# is a lightweight scripting language that looks like a less verbose C#,
and takes some additional inspiration from C++, Rust and JavaScript.

## Features

- A simple C++ API to add functions and types to the language and to
  embed the scripts into another program.
- A robust static type system that lets you write safer and faster code
  than alternatives like Lua.
- Uses reference counting instead of Garbage Collection, so the language
  does not have garbage collection stutter problems, helping performance for
  games.

## Example

```cs
using system::io

module example

fn getGreeting() -> string
{
    return "hello"
}

// The EntryPoint attribute tells the compiler this should be
// a function the runtime can call.
[EntryPoint]
fn helloWorld()
{
    const greeting = getGreeting()

    writeln($"{greeting} world!")
}
```

For more examples see the `examples/` directory.