from http.server import SimpleHTTPRequestHandler, HTTPServer

class SPAHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        print("getting---> ",self.path)
        if self.path == '/about':
            print("Serving about page")
            self.path = '/index.html' 
        return super().do_GET()
        

port = 8080
print(f"Serving on http://localhost:{port}")
HTTPServer(('', port), SPAHandler).serve_forever()
