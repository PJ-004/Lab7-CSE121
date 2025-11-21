from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import os

# 1. Setup Paths (Fixes path issues so CSS always loads)
BASE_DIR = Path(__file__).parent.resolve()
TARGET_FILE_PATH = BASE_DIR / 'location'

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

        # --- 1. Main Website ---
        if self.path == "/":
            self._set_headers()
            
            # Use your exact original HTML logic
            if TEMPS:
                # Create the paragraph content from TEMPS
                p_content = '\n'.join(i for i in TEMPS.split('\n'))
                
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
                                <p>{p_content}</p>
                            </div>
                            <script src="mainScript.js"></script>
                        </body>
                    </html>
                """
                self.wfile.write(bytes(html_content, "utf-8"))

            else:
                # Fallback HTML if no data yet
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
                self.wfile.write(bytes(html_content, "utf-8"))

        # --- 2. The /location Endpoint (for wget) ---
        elif self.path == "/location":
            try:
                # Open the file from the correct path
                with open(TARGET_FILE_PATH, 'rb') as f:
                    self._set_headers('text/plain')
                    self.wfile.write(f.read())
            except FileNotFoundError:
                self.send_error(404, "Location file not found (No data posted yet)")

        # --- 3. CSS Files ---
        elif self.path.endswith(".css"):
            # Look for the file in BASE_DIR, ignore the leading slash in path
            file_path = BASE_DIR / self.path.lstrip("/")
            if file_path.exists():
                with open(file_path, 'rb') as f:
                    self._set_headers('text/css')
                    self.wfile.write(f.read())
            else:
                self.send_error(404, f'CSS File Not Found: {self.path}')

        # --- 4. JS Files ---
        elif self.path.endswith(".js"):
            file_path = BASE_DIR / self.path.lstrip("/")
            if file_path.exists():
                with open(file_path, 'rb') as f:
                    self._set_headers('application/javascript')
                    self.wfile.write(f.read())
            else:
                self.send_error(404, f'JS File Not Found: {self.path}')

        else:
            self.send_error(404, 'File Not Found: %s' % self.path)

    def do_POST(self):
        global TEMPS
        global LOCATION
        
        # Read Data
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length)
        
        self._set_headers() # Send 200 OK
        
        if post_data:
            try:
                TEMPS = post_data.decode('utf-8')
                print(f"Data Received:\n{TEMPS}")
                
                # Write to file immediately
                lines = TEMPS.split('\n')
                if len(lines) >= 3:
                    LOCATION = lines[2]
                    with open(TARGET_FILE_PATH, 'w') as loc_file:
                        loc_file.write(LOCATION + "\n")
                        print(f"Updated location file: {LOCATION}")
                        print()
                else:
                    print("Data received, but not enough lines to extract location.")
                    
            except Exception as e:
                print(f"Error in POST: {e}")

if __name__ == "__main__":
    try:
        HTTPServer.allow_reuse_address = True
        with HTTPServer(('', 8000), Handler) as server:
            print(f"Starting Server in: {BASE_DIR}")
            print()
            server.serve_forever()
    except KeyboardInterrupt:
        print("Server Stopped")
