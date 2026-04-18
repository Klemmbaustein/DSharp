# D# standard library

The D# standard library is made up of multiple modules.

## Overview

- [system](#system-module) - Standard library root module
  - [err](#systemerr-module) - Error handling
  - [io](#systemio-module) - Input/output
  - [fs](#systemio-module) - Filesystem access

## `system` module

> Contains general purpose functions and attributes.

### Attributes
- `EntryPoint`:
  Tells the compiler to add this function to the programs entry points.
  On anything else it doesn't have any effect.
- `Reflect`:
  Tells the compiler to store reflection metadata for this function.
- `Discard`:
  Makes this function implicitly discardable.

### Functions

- `fn compareString(string str1, string str2) -> int`:
  Works like the C [strcmp](https://cplusplus.com/reference/cstring/strcmp/) function.

### Classes

- `class Map<K, V>` A tree-map that maps a value of type K to a value of type V

### Notes

  The `system` module contains many internal functions the language automatically generates calls for.

## `system::err` module

> Contains functions for raising and handling errors.

### Functions

- `fn abort()`:
  Stops program execution and prints a stack trace.

## `system::io` module

> Contains functions for reading and writing to files or the standard input/output streams.

### Functions

- `fn writeln(string toWrite)`:
  Writes the given string to the standard output, followed by a newline.

- `fn write(string toWrite)`:
  Writes the given string to the standard output.

- `fn writeInt(int toWrite)`:
  Writes a string representation of the given integer to the standard output.
 
- `fn readln() -> string`:
  Reads a line from the standard input and returns it.

- `fn popen(string command) -> system::io::File`:
  Creates a process and returns a file object that can be used to read from the standard output of that file.

### Classes

- `class File`:
  A class for reading or writing to a specific files.
  
  #### Constructors:
  - `fn new(string path)`:
    Creates a new file to read from with the given path.
 
  #### Methods:
  - `fn readln() -> string`:
    Reads a line from this file and returns it.
  - `fn isEmpty() -> bool`:
    Returns `true` if the file is empty, `false` if not.

## `system::fs` module

> Contains functions for accessing the file system.

### Functions

- `fn writeln(string toWrite)`:
  Writes the given string to the standard output, followed by a newline.

- `fn write(string toWrite)`:
  Writes the given string to the standard output.

- `fn writeInt(int toWrite)`:
  Writes a string representation of the given integer to the standard output.
 
- `fn readln() -> string`:
  Reads a line from the standard input and returns it.

- `fn popen(string command) -> system::io::File`:
  Creates a process and returns a file object that can be used to read from the standard output of that file.

### Classes

- `class Path`:
  A filesystem path.
  
  #### Constructors:
  - `fn new(string path)`:
    Creates a path object from the given string.
 
  #### Methods:
  - `fn readln() -> string`:
    Reads a line from this file and returns it.
  - `fn isEmpty() -> bool`:
    Returns `true` if the file is empty, `false` if not.
