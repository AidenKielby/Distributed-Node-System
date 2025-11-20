program basics:
* support for both java and python worker code
* made in C++
* Master and Clients
* Master sends clients code with supported languages
* outermost "return" will send the output bsck to the Master

Startup:
* starts by asking what languages are installed on the divice
* if no supported languages are on the divice then it will end the client and say why
* otherwise it will only apply for "jobs" that use that language
* ask if user wsnts the program to use CPU, GPU, or both
