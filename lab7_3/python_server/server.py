from http.server import BaseHTTPRequestHandler, HTTPServer
import logging
import os

TEMPS = None
LOCATION = None

class Handler(BaseHTTPRequestHandler):
    def _set_headers(self, content_type='text/html'):
        self.send_response(200)
        self.send_header('Content-type', content_type)
        self.end_headers()

    def do_GET(self):
        global TEMPS
        global LOCATION
        if self.path == "/":
            self._set_headers()
            #f = open("index.html", "r")
            if TEMPS:
                html_content = f"""
                    <!DOCTYPE html>
                    <html lang="en">
                        
                        <head>
                            <meta charset="UTF-8">
                            <meta name="viewport" content="width=device-width, initial-scale=1.0">
                            <title>Temperature Website</title>
                            <link rel="stylesheet" href="style.css"/>
                        
                            <meta http-equiv='cache-control' content='no-cache'>
                            <meta http-equiv='expires' content='0'>
                            <meta http-equiv='pragma' content='no-cache'>
                        </head>
                        
                        <div id="Header">
                            <h1 id="dateToday"></h1>
                        </div>
                        
                        <body id="main">
                        
                            <div id="body">
                                <p>{'\n'.join(i for i in TEMPS.split('\n'))}</p>
                            </div>
                        
                            <script src="mainScript.js"></script>
                        
                        </body>
                        
                    </html>
                """
                with open('location', 'w') as loc:
                    loc.write(TEMPS.split("\n")[2]);
                print(TEMPS)
                LOCATION = TEMPS.slit('\n')[2]
                print(LOCATION)
                self.wfile.write(bytes(html_content, "utf-8"))

            else:
                html_content = """
                    <!DOCTYPE html>
                    <html lang="en">
                        
                        <head>
                            <meta charset="UTF-8">
                            <meta name="viewport" content="width=device-width, initial-scale=1.0">
                            <title>Temperature Website</title>
                            <link rel="stylesheet" href="style.css"/>
                        
                            <meta http-equiv='cache-control' content='no-cache'>
                            <meta http-equiv='expires' content='0'>
                            <meta http-equiv='pragma' content='no-cache'>
                        </head>
                        
                        <div id="Header">
                            <h1 id="dateToday"></h1>
                        </div>
                        
                        <body id="main">
                        
                            <div id="body">
                                <p>Current Temperature and Humidity: N/A</p>
                            </div>
                        
                            <script src="mainScript.js"></script>
                        
                        </body>
                        
                    </html>
                """
                print(TEMPS)
                self.wfile.write(bytes(html_content, "utf-8"))
        elif self.path.endswith(".css"):
            try:
                # Open the CSS file and serve it
                with open(os.path.join(os.getcwd(), self.path[1:]), 'rb') as f:
                    self._set_headers('text/css')
                    self.wfile.write(f.read())
            except FileNotFoundError:
                self.send_error(404, 'File Not Found: %s' % self.path)
        elif self.path.endswith(".js"):
            try:
                # Open the JavaScript file and serve it
                with open(os.path.join(os.getcwd(), self.path[1:]), 'rb') as f:
                    self._set_headers('application/javascript')
                    self.wfile.write(f.read())
            except FileNotFoundError:
                self.send_error(404, 'File Not Found: %s' % self.path)
        else:
            self.send_error(404, 'File Not Found: %s' % self.path)
        #self.wfile.write(f.read())

    def do_HEAD(self):
        self._set_headers()

    def do_POST(self):
        global TEMPS
        self._set_headers()
        self.data_string = self.rfile.read(int(self.headers['Content-Length']))

        self.send_response(200)
        self.end_headers()

        if 'content-type' in self.headers:
            TEMPS = self.data_string.decode('utf-8')
        
        print(self.data_string.decode('utf-8'))

if __name__ == "__main__":
    try:
        with HTTPServer(('', 8000), Handler) as server:
            print("Starting Server")
            server.serve_forever()
    except KeyboardInterrupt:
        print("Server Stopped")
