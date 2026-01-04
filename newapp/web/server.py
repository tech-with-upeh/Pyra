from http.server import HTTPServer, SimpleHTTPRequestHandler
from socketserver import ThreadingMixIn
import os
import mimetypes

# Ensure WASM files have correct MIME
mimetypes.add_type("application/wasm", ".wasm")

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""
    pass

class SPAHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        # Translate path
        path = self.translate_path(self.path)

        # SPA fallback
        if not os.path.exists(path) or os.path.isdir(path):
            self.path = "/index.html"

        return super().do_GET()

if __name__ == "__main__":
    # os.chdir("public")  # Your build output folder
    server = ThreadedHTTPServer(("0.0.0.0", 9000), SPAHandler)
    print("Serving on http://0.0.0.0:9000 (multi-threaded)")
    server.serve_forever()
