# cppm - C++ project manager
### Don't write config files, write C++!

> [!IMPORTANT]
> cppm is only tested on Linux and more than likely won't work as intended on other OS.

### Structure created
- `📁 src` - **your code goes here**
	- `[packageName].cpp` - entry point of your application with a simple "Hello World" application
	- `[packageName].h` - header file for the entry point
- `📁 includes` *(all dependencies you include will be placed here, **never create files here!**)*
	- `📁 src` *(dependency include files)*
	- `📁 lib` *(dependency lib files)*
	- `📁 uses` *(system-wide dependencies placed here, see `cppm help use`)*
- `📁 tests` *(test files will be created here, see `cppm help test`)*
- `📁 .git` *(directory created by git)*
- `premake5.lua` *(describes your project to the compiler, **don't edit manually!**)*
- `.gitignore` *(files/directories that won't be pushed to remote, does not apply to all listed, see `cppm help push`)*
- `README.md`

You should only ever need to edit files within ./src directory. You can create directories within src if you need additional structure in your project.\

> [!CAUTION]
> **Never create files in `includes` directory!** They will get **deleted** when some of the cppm commands are executed, everything within this directory is managed by cppm.

> [!CAUTION]
> **Never edit `premake5.lua` manually**. Your changes will be **overwritten** by cppm whenever it regenerates the file.


### Package
A package is an application or a library. Package can be **managed** *(your code)* or **non-managed** *(3rd party code)*. Managed packages are created using the `init` command, non-managed packages are not created, you register a directory as a non-managed package using `cppm register`, run `cppm help register` to learn how to use the command.

### Quickstart
Create a new package:\
`cppm init [packageName]`

Add source files to package:\
`cppm add [srcName]`

Include another package as a dependency:\
`cppm include [packageName]`

Compile package:\
`cppm build`

List all available commands:\
`cppm help`