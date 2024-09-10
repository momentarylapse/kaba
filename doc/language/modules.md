# Modules and packages

Each kaba source file is a _module_.

## Importing from other files

Given a source file `source.kaba` with contents
```kaba
var a: i32

class X
    ...

func f()
    ...
```
you can then _import_ those symbols and use them from another source file (skip `.kaba`):
```kaba
use source.f   # individually
use source.*   # or all

f()
```

Or you can import the whole _namespace_:
```kaba
use source

source.f()
```

## Locating other files

If you have a tree of source files
```
base/
    first.kaba
    a/
        second.kaba
    b/
        third.kaba
        me.kaba
```

From within `me.kaba`, import:
```kaba
use first
use a.second
use third
```

🔥 Rule: files are automatically searched in the current directory and "several" of its parents (i.e. `base/b/` then `base/`).

🔥 Other directories need to be added with dots (`base.a.second`) relative to the common parent.

## Packages

Multiple modules in one directory can be treated as one unit, a _package_.

The directory must contain a module with the same name:
```
a/
    a.kaba
    other.kaba
```

Now a simple
```kaba
import a
```
is enough. `a.kaba` _should_ export all relevant symbols...

🔥 The exact rules are still under development!


## Internal packages

The kaba compiler provides several packages by default.

`base.*` and `math.*` are automatically imported.

Others include file management (`os`), image loading (`image`), networking (`net`), graphical user interfaces (`hui`), OpenGl (`gl`) and Vulkan (`vulkan`).

See: https://wiki.michi.is-a-geek.org/kaba.reference/


