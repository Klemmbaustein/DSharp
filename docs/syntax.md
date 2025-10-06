# Language syntax

## Overview

1. [Files](#files)
2. [Modules](#modules)
3. [Functions](#functions)
4. [Scopes](#scopes)
5. [Statements](#statements)
    - [If-statements](#if-statements-if-condition-scope)
    - [Variable definition statements](#variable-definition-statements-type-name-or-type-name-value)
    - [While-statements](#while-statements-while-condition-scope)
6. [Expressions](#expressions)
    - [Literals](#literals)
    - [Variables](#variables)
    - [Operators](#operators)
        - [Unary](#unary)
        - [Ternary](#ternary)
    - [Discardable expressions](#discardable-expressions)
7. [Classes](#classes)
    - [Class methods](#class-methods)
8. [Attributes](#attributes)

## Files

A file consists of:

- The module name

  With the syntax:
  `module [moduleName]` where [moduleName] is the full name of the file's module.
  Any functions or classes in the file will be accessible in this module for other files.
  A file can only contain a single module name, and that module name will be used for the entire file.

  Modules can be nested. For example `myModule::mySubModule` is a valid name, and from a module called
  `myModule` or anything using that module, the contents of `myModule::mySubModule` will be accessible as only
  `mySubModule`.

  Module definitions can span over multiple files if multiple files have the same module name.

- Usings
  
  With the syntax:
  `using [moduleName]` where [moduleName] is the full name of any module.

- Functions

  See the Functions section

- Classes

## Modules

A module in D# is similar to a namespace in C++. All classes and functions in them will be accessible

## Functions

A function in D# has the form:

```
[modifier] fn [name] ([arguments]) [scope]
```

or if the function has a return type:

```
[modifier] fn [name] ([arguments]) -> [returnType] [scope]
```

- [modifier] can be either `virtual` or `overriode` if the function
  is a class method.
  For more information see the Class methods section.
- [name] can be any valid name. A name can't start wit a digit and
  otherwise can only contain letters and digits.

## Scopes

A scope is a collection of statements, surrounded by {} Scopes can also be nested if a statement contains another scope.
In that case the sub scope will have access to any variables of the parent scope.

Example of a scope:

```
{
    const number = 5
    writeInt(number)
    return
}
```

## Statements

A statement is a block of code that does something.

Statements are separated by new lines, unless a (), [] or {} bracket pair is open.

So for example: `a \n b` isn't a single statement, but `a ( \n b )` is.

Types of statements are:

- ### If-statements: `if [condition] [scope]`
  
  [condition] is an expression evaluating to the `bool` type. If this condition evaluates to `true`,
  the scope will be run.

  An if-statement can optionally be followed with an else-statement: `else [scope]`.

  If the condition of the last if-statement evaluated to `false`, the scope of the else-statement will be run.
  
  [scope] can otherwise also be another if-statement, for example:

  ```
  if value == 1
  {
      // a
  }
  else if value == 2
  {
      // b
  }
  ```

- ### While-statements `while [condition] [scope]`
  
  Like an if-statement, [condition] is an expression evaluating to the `bool` type. If this condition evaluates
  to `true`, the scope will be run.

  At the end of the scope, the statement will be repeated. If [condition] still evaluates to `true`, the scope
  will be run again.

- ### Variable definition statements `[type] [name]` or `[type] [name] = [value]`
  
  A variable definition defines a variable in the scope.

- ### For-statements `for [varDef] in [arrayExpression] [scope]`

  TODO:
- ### Expression statements
  A statement can also just be an [expression](#expressions).
  
## Expressions

An expression is piece of code that results in a value.

Types of expressions are:

- ### Literals
  A literal value of a type.
  - #### Number literals:
    Any integer number `0`, `-15`, etc. Has the type `int` if it doesn't contain a decimal part or `float` if it does.
  - #### String literals
    A string of characters, with the syntax `"[text]"`, where [text] can be any combination of characters.
    Has the type `string`.

    String literals can also be interpolated, with the syntax `$"[interpolatedString]"`.
    In a string literal, anything in `{}`-braces will be treated as an expression that's cast to a string,
    and will be inserted into the string at that place.
    
    As an example: The string `$"1 + 1 = {1 + 1}"` evaluates to `1 + 1 = 2` at runtime.
  - #### Boolean literals
    Either `true` or `false`, evaluate to their `bool` type equivalents.
  - #### Function references
    Writing just the name of a function will be interpreted as a function reference.
- ### Variables
  A variable expression reads the value of a variable that was earlier defined with a
  [variable definition statement](#variable-definition-statements-type-name-or-type-name-value).
  The expression will have the same type as the one the variable has been declared as.

- ### Function calls

  A function call follows the form `[name] ([arguments])`, where [name] is the name of a function
  and [arguments] is a comma-separated list of argument expressions, where the number of expressions given
  to the function must match the number of arguments the function has and each expression must be convertible
  to the matching argument's type defined in the function.

- ### Operators
  Operators chain together or modify expressions. The behavior of an operator changes depending on
  which types the operator is being used with. There are two kinds of operators:
  - #### Unary
    Unary operators only operate on a single expression, and are written before the expression.
    Unary operators are:
    - `not` operator - returns the opposite value of a Boolean. Other standard types do not use it.
    
    - `*` (dereference) operator - Performs a null check a nullable type, then converts it
      to it's non nullable version.
    
    - `-` (unary minus) operator - Converts a number to the negative value of itself.
      As an example, `-(5)` is equal to `-5` (the literal version)
  - #### Ternary
    Ternary operators operate on two expressions. The operator goes between these 2 expressions, for example
    `1 + 2` contains the two expressions `1` and `2`, and the `+` operator combines these two expressions.
    - `+`, `-`, `*`, `/`, `%` (add, subtract, multiply, divide, modulo) - Apply their respective
      mathematical operations to the value.
    - `and`, `or` logical operators - performs these logical operators

### Discardable expressions

An expression is discardable if it doesn't have any resulting type (such as functions returning no type)
or if it is a function or variable marked with the `[system::Discard]` attribute.

## Classes

A class in D# has the form

```
class [name] : [superClass]
{
    [methods or members]
}
```

### Class methods

A class method looks like a function, but can have the `virtual` and
`override` modifiers. All class methods have access to the special
`this` variable which accesses the class instance on which the method has been called on.
The `this` variable is constant and cannot be written to.

Example:

```
class X
{
    int number = 5

    fn method()
    {
        this.number += 1
    }
}
```

However, in a class method, any expression that does not have another meaning will implicitly be interpreted
as a member of the `this` variable.