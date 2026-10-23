Specifies if the binary has dynamic debugging support.

```lua
dynamicdebugging ("value")
```

### Parameters ###

`value` is one of:

| Value | Description | Notes |
|-------|-------------|-------|
| Default | Uses the default dynamic debugging behavior. | |
| On | Allows dynamic debugging of source code. | For MSVC, debug [symbols](symbols.md) must be present. [Edit and Continue](editandcontinue.md) is not allowed. For a full list of flags that are required, see the [Microsoft Docs](https://learn.microsoft.com/en-us/cpp/build/reference/dynamic-deopt?view=msvc-170). |
| Off | Disallows dynamic debugging of source code. | |

### Applies To ###

Project configurations.

### Availability ###

Premake 5.0.0 or later for Visual Studio 2022 and later or MSC toolsets.

### Examples ###

```lua
dynamicdebugging "On"
```

### See Also ###

- [C++ Dynamic Debugging](https://devblogs.microsoft.com/cppblog/cpp-dynamic-debugging-full-debuggability-for-optimized-builds/)
