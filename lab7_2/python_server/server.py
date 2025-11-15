from http.server import BaseHTTPRequestHandler, HTTPServer
import logging

class Handler(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        self._set_headers()
        f = open("index.html", "r")
        self.wfile.write(f.read())

    def do_HEAD(self):
        self._set_headers()

    def do_POST(self):
        self._set_headers()
        self.data_string = self.rfile.read(int(self.headers['Content-Length']))

        self.send_response(200)
        self.end_headers()

        print(self.data_string.decode('utf-8'))

if __name__ == "__main__":
    try:
        with HTTPServer(('', 8000), Handler) as server:
            print("Starting Server")
            server.serve_forever()
    except KeyboardInterrupt:
        print("Server Stopped")
