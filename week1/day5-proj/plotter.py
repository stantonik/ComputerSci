
from http.server import BaseHTTPRequestHandler, HTTPServer
import serial, sys, os, json

TAG = "plotter.py"

logfile = None
ser = None

def config_serial():
    serial_port = None
    baudrate = 9600
    global ser
    try:
        if len(sys.argv) != 3: raise Exception
        serial_port = sys.argv[1]
        baudrate = int(sys.argv[2])

        if not os.path.exists(serial_port): raise Exception("port")
    except Exception as e:
        if e.args == "port":
            print(TAG + ": '{}' not found.".format(serial_port))
        else:
            print(TAG + ": syntax error")
            print("usage: " + TAG + " [port] [baudrate]")
        exit()
    finally:
        ser = serial.Serial(serial_port, baudrate=baudrate, timeout=1)

class MyRequestHandler(BaseHTTPRequestHandler):

    # Handle GET request
    def do_GET(self):
        file_path = 'index.html'
        
        if os.path.exists(file_path):
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            
            with open(file_path, 'rb') as file:
                content = file.read()
            self.wfile.write(content)
        else:
            self.send_response(404)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b"404 Not Found")
    
    # Handle POST request
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        try:
            data = json.loads(post_data.decode('utf-8'))
            print(f"Received data: {data}")
            
            # Send a 200 OK response
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()

            # Respond with a JSON message
            line = ""
            if ser is not None: 
                line = ser.readline().decode('utf-8').strip()
                print(line)
                if line:
                    print(f"Received:", line)
                    response = {'status': 'success', 'data': line}
                    self.wfile.write(json.dumps(response).encode('utf-8'))
                else:
                    response = {'status': 'success', 'data': 0}
                    self.wfile.write(json.dumps(response).encode('utf-8'))
            else:
                response = {'status': 'error', 'message': 'Serial port not initialized'}
        
        except json.JSONDecodeError:
            # Send a 400 Bad Request response if JSON is invalid
            self.send_response(400)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({'error': 'Invalid JSON'}).encode('utf-8'))

def run_server():
    port = 1256
    server_address = ('', port)
    httpd = HTTPServer(server_address, MyRequestHandler)
    print(TAG + f": starting server on the ip : http://localhost:{port}...")
    httpd.serve_forever()

def start_serial_sniffer():
    global logfile
    logfile = open("log.txt", "a")
    config_serial()

try:
    start_serial_sniffer()
    run_server()
except KeyboardInterrupt:
    print("");
    print(TAG + ": stopping...")




