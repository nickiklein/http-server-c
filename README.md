# HTTP Server in C

The purpose of this project is to code a HTTP Server from scratch in C.

One of the main motivations of this personal project is to implement everything without the help of any AI coding tools.
AI can and should be used as a learning tool to help with understanding, however it shall never write code for me.

The key resources used for this project are Beej's guides, namely:

- Beej's Guide to Network Programming (`https://beej.us/guide/bgnet/html/split/index.html`)
- Beej's Guide to C Programming (`https://beej.us/guide/bgc/html/split/`)

Additionally a motivation is to try and work with and learn VIM motions.

## Milestones

These milestones were created with Claude. After having finished a very first working server, I told Claude to give me a milestones list for me to follow along and track my progress.
Each milestone has a **Done when** check so it's clear when to tick it off.

### Phase 0: Tooling

- [ ] **Build setup**: a `Makefile` (or build script) that compiles with `-Wall -Wextra` and optionally `-fsanitize=address,undefined` for a debug build.
  *Done when:* `make` builds the server and `make debug` builds it with sanitizers.

### Phase 1: TCP foundation

- [x] **Listening socket**: resolve the address with `getaddrinfo`, then `socket`, `setsockopt`, `bind`, `listen`.
- [x] **First connection**: `accept` a client, `send` a message, `recv` a reply.
  *Done when:* `nc localhost 3000` shows the welcome message. ✅
- [ ] **Serve forever**: the server keeps running and accepts client after client in a loop, instead of exiting after the first one.
  *Done when:* you can connect with `nc`, disconnect, and connect again without restarting the server.
- [ ] **Know your client**: print the connecting client's IP address and port.
  *Done when:* each connection logs something like `Connection from 127.0.0.1:52344`.
- [ ] **Reliable sending**: a `send_all()` helper that keeps calling `send` until every byte is out.
  *Done when:* a large message (e.g. 1 MB) arrives completely on the client side.
- [ ] **Reliable receiving**: read in a loop until you have a complete message, instead of one `recv` into a fixed buffer.
  *Done when:* a message larger than your buffer is received in full.

### Phase 2: Speak HTTP

- [ ] **Hello, browser**: answer every request with a minimal valid HTTP response (status line, headers, blank line, body).
  *Done when:* `curl -v localhost:3000` and a browser both show your page.
- [ ] **Parse the request line**: extract method, path and HTTP version from the first line.
  *Done when:* the server logs `GET /about HTTP/1.1` for a request to `/about`.
- [ ] **Parse headers**: read header lines into a data structure, ending at the empty line (`\r\n\r\n`).
  *Done when:* you can look up the `Host` and `User-Agent` values of a request.
- [ ] **Routing**: different paths return different responses, and unknown paths return `404 Not Found`.
  *Done when:* `/` and `/about` return different pages and `/nope` returns 404.
- [ ] **Error responses**: return `400 Bad Request` for garbage input and `405 Method Not Allowed` for unsupported methods.
  *Done when:* typing nonsense into `nc` gets a 400 instead of a crash or a hang.
- [ ] **Correct headers**: every response has an accurate `Content-Length` and `Content-Type`.
  *Done when:* `curl -v` shows both, with correct values.

### Phase 3: Serve real files

- [ ] **Static files**: map the request path to a file inside a document root folder (e.g. `./public`) and send its contents.
  *Done when:* an HTML page with a linked CSS file and image loads correctly in the browser.
- [ ] **MIME types**: set `Content-Type` from the file extension (`.html`, `.css`, `.js`, `.png`, `.jpg`, …).
  *Done when:* images and stylesheets render instead of showing as raw text or downloading.
- [ ] **Directory index**: a request for a directory serves its `index.html`.
  *Done when:* `localhost:3000/` serves `public/index.html`.
- [ ] **Path traversal protection**: requests can never escape the document root.
  *Done when:* `curl --path-as-is localhost:3000/../../etc/passwd` gets a 403 or 404.
- [ ] **Request bodies**: read a body based on `Content-Length` (e.g. a `POST` with form data) and echo it back.
  *Done when:* `curl -d "name=nicolas" localhost:3000/echo` returns the body you sent.

### Phase 4: Concurrency

- [ ] **Process per client**: handle each connection in a child process via `fork()`.
  *Done when:* two `nc` sessions can be connected and served at the same time.
- [ ] **Thread per client**: same as above, but with `pthread`s. Compare the two approaches.
  *Done when:* concurrent clients work with threads, and you can explain the trade-offs between processes and threads.
- [ ] **Event loop**: one thread serving many clients with `poll()` (and later maybe `epoll`/`kqueue`).
  *Done when:* many simultaneous clients are served with no `fork` or threads.
- [ ] **Keep-alive**: support persistent connections, i.e. multiple requests over one TCP connection.
  *Done when:* `curl -v localhost:3000/ localhost:3000/about` reuses the connection (`Re-using existing connection` appears in the output).
- [ ] **Timeouts**: drop clients that are idle or too slow.
  *Done when:* an `nc` session that sends nothing gets disconnected after N seconds.

### Phase 5: Robustness

- [ ] **Graceful shutdown**: on Ctrl+C (`SIGINT`), stop accepting, finish or close open connections, and release every resource.
  *Done when:* shutdown prints a clean message and Valgrind or ASan reports no leaks.
- [ ] **Limits**: cap header and body sizes, and respond with `413` / `431` when they're exceeded.
  *Done when:* sending a 100 MB header gets an error response instead of eating memory.
- [ ] **Access log**: one log line per request (time, client IP, method, path, status, response size).
  *Done when:* the log looks like a simple version of nginx's access log.
- [ ] **Configuration**: port and document root set via command-line arguments.
  *Done when:* `./server --port 8080 --root ./site` works.
- [ ] **Load test**: benchmark with `wrk` or `ab` and compare your concurrency models from Phase 4.
  *Done when:* you have requests/sec numbers for each model written down in this README.

### Phase 6: Stretch goals

- [ ] **Caching**: `Last-Modified` / `If-Modified-Since` → `304 Not Modified`, then `ETag` / `If-None-Match`.
- [ ] **Range requests**: `Range` header → `206 Partial Content` (lets you seek in videos).
- [ ] **Chunked transfer encoding**: stream responses without knowing the size upfront.
- [ ] **Compression**: gzip responses when the client sends `Accept-Encoding: gzip` (via zlib).
- [ ] **CGI or dynamic routes**: run a script or a C handler function to generate responses.
- [ ] **HTTPS**: TLS with OpenSSL.
  *Done when:* `curl -k https://localhost:3443` works.
- [ ] **WebSockets**: handle the `Upgrade` handshake and exchange framed messages.
