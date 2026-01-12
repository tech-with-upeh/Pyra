# Helios v1.0.0

Helios is a full-stack framework that compiles `.ink` files into **web applications**, with future support for **Android, iOS, and desktop**.  
It uses a custom C++ backend with Lexer, Parser, Semantic Analysis, and a Virtual DOM system for reactive UIs.

---

## Features

- Compile `.ink` files into web apps (HTML, JS, WASM) `for v1`
- Virtual DOM system for efficient UI updates  
- Test Apps  (`examples/myapp`, `examples/newapp`, etc.)  
- Modular C++ code for fast development and incremental builds via CMake  
- Test folder for experimental apps and WebSocket/server testing

---

## Prerequisites

- **Windows / Linux / macOS**  
- **C++17 compatible compiler** (g++, clang++, MSVC)  
- **CMake** (>= 3.15)   
- Optional:

       **Emscripten SDK** for web builds
      **Android SDK** for android builds
      **Xcode** for ios builds 

---

## Building Helios

1. Create a build directory and navigate to it:

```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
./helios.exe   # Windows
./helios       # Linux/macOS
```

## Usage 

1. ` helios create <yourappname> ` - creates folder and file structure
2. `helios dev target (web | android | ios)` - starts a development server for specified target
3. `helios run target (web | android | ios)` - starts a production server for specified target
4. `helios build target (web | android | ios)` - starts a production build for specified target
5. `helios clean` - cleans folder structure


## Contributing

- Fork the repository
- Create a feature branch: git checkout -b feature/my-feature
- Commit your changes: git commit -m "Add my feature"
- Push to branch: git push origin feature/my-feature
- Open a pull request

## License

  i dont know!
  if you steal, abeg give me credit

## Roadmap / Todo 

- Android, iOS, and desktop app support

- Improve virtual DOM performance

- Add more examples and documentation

- Optimize incremental build system

  ## Author

  Wisdom Upeh
