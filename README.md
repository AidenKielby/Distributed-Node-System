# PCTree — Distributed Node System

PCTree is an experimental, minimal distributed task runner written in C++ for Windows. The idea is intentionally small and educational:

- A central host (server) sends a Python file and a function name to a connected client.
- The client saves the file, runs the named function (allows arguments, no parenthasis if no arguments in function), then returns the function's result (stringified) to the host.

This repo includes:

- `Master.cpp` — server/host program
- `Client.cpp` — worker/client program (uses embedded CPython)
- `testThings` — example Python tasks

---

## Quick start

1) Build `Master.exe` and `Client.exe` using your C++ toolchain.
2) Start `Master.exe` (server) — it listens on port 8080.
3) Start one or more `Client.exe` instances — they will connect to the server and send a 64-byte hostname.
4) On the server, use `client_list` to see clients, then enter the client's index to send a file and function name.
# or
1) make sure you have Python installed, the prebuilt .exe expects python 3.14
2) run the master and client .exe files

---

## Files of interest

- `Master.cpp` — listens for clients, sends tasks, prints results
- `Client.cpp` — receives file+function, saves file to `newFile.py`, calls the Python function, returns result
- `test_task.py` — example file with several simple functions for testing

---

## Build notes (Windows / VS Code)

This workspace already includes helpful VS Code configuration and a build task for convenience. Key details:

- The C++ Intellisense config is stored in `.vscode/c_cpp_properties.json`. In this workspace it contains an include path to Python headers so the editor can resolve Python C API symbols:

	- Example include path from workspace: `C:/Users/aiden/AppData/Local/Programs/Python/Python314/include`

- The project also has a build task (visible as **C/C++: clang++.exe build with Python**) that compiles a single .cpp file and links to the local Python libs. The task's clang++ args in this workspace show how to supply include and lib locations for Python.

If you have clang available, a command equivalent to the workspace task looks like this (PowerShell):

```powershell
C:/Users/aiden/AppData/Local/Programs/Swift/Toolchains/6.2.0+Asserts/usr/bin/clang++.exe \
	-std=c++17 -g \
	-IC:/Users/aiden/AppData/Local/Programs/Python/Python314/include \
	Master.cpp -LC:/Users/aiden/AppData/Local/Programs/Python/Python314/libs -lpython314 \
	-o Master.exe

C:/Users/aiden/AppData/Local/Programs/Swift/Toolchains/6.2.0+Asserts/usr/bin/clang++.exe \
	-std=c++17 -g \
	-IC:/Users/aiden/AppData/Local/Programs/Python/Python314/include \
	Client.cpp -LC:/Users/aiden/AppData/Local/Programs/Python/Python314/libs -lpython314 \
	-o Client.exe
```

Notes:

- Replace `python314` with the correct library name for your Python installation if it differs (for example `python310`, `python39`, etc.).
- When linking to Python with clang on Windows, you often need to pass the proper library directory (`-L`) and library (`-lpythonXXX`).

If you prefer MSVC (`cl.exe`), update the `cl` invocation and the linker to point at Python headers and libraries. Example (PowerShell using MSVC environment):

```powershell
cl /std:c++17 /EHsc Master.cpp ws2_32.lib

cl /std:c++17 /EHsc Client.cpp /I "C:\Users\aiden\AppData\Local\Programs\Python\Python314\include" \
	 /link /LIBPATH:"C:\Users\aiden\AppData\Local\Programs\Python\Python314\libs" python314.lib ws2_32.lib
```

Be sure to open the Developer Command Prompt for VS so `cl.exe` and linkers are available or configure your VS Code tasks accordingly.

---

### Tips for editor integration

- `c_cpp_properties.json` tells the editor where to find headers. In this workspace `compilerPath` is `cl.exe` and `cppStandard` is `c++17`.
- If you get unresolved symbol errors during editing, ensure the Python include path shown above points to an installed Python version on your system.
- If you need help turning these into a reproducible build, I can add a small `CMakeLists.txt` or `tasks.json` entries to build both executables and configure Python paths automatically.

---

## Protocol (summary)

Client → Server (on connect):
- 64 bytes: hostname (null-padded)

Server → Client (task):
- 4 bytes: file size (uint32 network order)
- N bytes: function name (null-terminated string)
- file contents

Client → Server (result):
- 4 bytes: result size (uint32 network order)
- N bytes: UTF-8 result string

---

## Example `test_task.py`

This repository already includes `test_task.py` (linked below). It contains several test functions you can run:

- `hello()` — returns a short string
- `compute()` — small computational example
- `slow()` — sleeps for 2s and returns a string
- `system_info()` — returns a dict of system info
- `random_number()` — returns a random int string

Open and inspect `test_task.py` in this repo to see examples and adjust them as needed. Link: [`./test_task.py`](./test_task.py)

---

## Safety & limitations

⚠️ Important: clients execute arbitrary Python code received from the server. Only run clients with trusted servers. Consider implementing sandboxing, authentication, and encryption before using this outside experiments.

Current limitations:

- no authentication or encryption
- simple synchronous protocol (no streaming/resume)

---

## Python API (`pctree.pyd`)

PCTree also provides a Python API via the prebuilt `pctree.pyd` in the `PythonAPI` folder. This allows you to interact with the master server entirely from Python, without compiling C++ code.

### Usage

```python
import pctree

# Start the master server
pctree.start_master()

# List connected clients
clients = pctree.client_list()
print(clients)  # Returns a string listing connected clients

# Send a Python file to a specific client and execute a function
result = pctree.send_file(client_index=0, file_path="test_task.py", function_name="hello")
print(result)  # Prints the stringified result of the function

# Ping a client to check if it's still responsive
pctree.ping_client(client_index=0)

# Stop the master server
pctree.kill_master()
```
### API Functions
| Function | Description |
|:-------------|:--------------|
| ```start_master()``` | Starts the master server, listening on port 8080. |
| ```client_list()``` | Returns a string of all currently connected clients and their identifiers. |
| ```send_file(client_index, file_path, function_name)``` | Sends a Python file to a connected client and executes the specified function. Returns the function result as a string. |
| ```ping_client(client_index)``` | Sends a ping to a client to verify that it is still connected. |
| ```kill_master()``` | Stops the master server and closes all client connections. |

### Notes
 - The Python API uses the same protocol as the C++ executables.
 - Clients execute arbitrary Python code received from the server. Only use trusted clients and servers.
 - The API is compatible with Python 3.14 by default (matches the prebuilt .pyd). Make sure your Python environment matches the compiled version.
 - Multiple clients can be connected and addressed by their index in client_list().

---

## Ideas / TODO

- JSON task lists, eg.
```JSON
{
	"problems": {
		[
			"client": 0,
			"file": "testThings/mandelbrot.py",
			"function": "demo(300,200,50)"
		],
		[
			"client": 1,
			"file": "testThings/maze.py",
			"function": "maze(10,10)"
		]
	}
}
```
- use that ^^ to make a pasword decrypter using a keylist file and spit the file into chunks and send to clients

