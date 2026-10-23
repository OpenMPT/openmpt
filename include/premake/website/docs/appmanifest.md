Adds the ApplicationManifest property to .NET projects

```lua
appmanifest ("path")
```

### Parameters ###

`path` should contain the path to the application manifest file.

## Applies To ###

.NET project configurations.

### Availability ###

Premake 5.0.0

### Examples ###

If the app manifest file is called `app.manifest` and is located in the folder alongside the project then the following usage will reference it within the project:

```lua
appmanifest "app.manifest"
```
