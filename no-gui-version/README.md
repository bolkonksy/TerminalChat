# TerminalChat
A simple chat application for Linux written in C language using ncurses library for basic GUI.

### How to use:
1. Run `make all` to compile everything
2. Start the server: 

  `./server [port]`
3. Connect with clients: 

  `./client [server name] [port]`
  
If server is running on a local machine, you can use `localhost` for the server name. Try hosting server with any port in range 1024-65536. 
