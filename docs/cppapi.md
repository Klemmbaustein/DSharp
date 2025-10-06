# D# C++ API

Scripts can be compiled, loaded and run from C++ code.
C++ can also add modules with types and functions to the language context.

## Language contexts and libraries

The scripting language shares state between the runtime and compiler in the
`ds::LanguageContext` class, like what modules are loaded, the version of the language,
etc.

The `ds::LanguageContext` class is just a regular C++ class that can be constructed.

```cpp
#include <ds/language.hpp>

using namespace ds;

LanguageContext language = LanguageContext();
```

You can use the `ds::LanguageContext::addNativeModule(NativeModule*)` method to add a module
to the language context. The standard library also provides a function
`ds::modules::registerStandardLibrary`, which adds all built in standard library modules to the context.
Individual standard library modules can be created using their `createModule()` function.


## Compiling code

```cpp
using namespace ds;

LanguageContext language;
modules::registerStandardLibrary(&language);

ParseContext* compiler = language.createCompiler();
compiler->addFile("test.ds");
BytecodeStream compiled = compiler->compile();
delete compiler;
```