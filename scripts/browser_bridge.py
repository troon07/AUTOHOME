"""Local browser transport bridge. Never infer occupancy, speech or actions here.

Loopback binding keeps microphone access in a secure browser context. The
browser still sends its PCM/input bytes to the ESP32, where processing belongs.
No token injection, host control logic, fabricated board state or cloud calls.
"""
import argparse
import http.client
import ipaddress
import json
import threading
import time
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit


def run(host, port, serial_port=None, preview_only=False):
    board = str(ipaddress.IPv4Address(host))
    connection=None; serial_lock=threading.Lock(); request_id=0
    if serial_port:
        import serial
        connection=serial.Serial(port=None,baudrate=115200,timeout=.1,write_timeout=6)
        connection.dtr=False; connection.rts=False; connection.port=serial_port; connection.open()

    def send_serial(method,path,body,token):
        nonlocal request_id
        with serial_lock:
            request_id=(request_id%0xffffffff)+1
            token=token or '-'
            if len(token)>63 or any(c.isspace() for c in token): raise ValueError('Invalid token format')
            packet=f'@{request_id} {"G" if method=="GET" else "P"} {path} {len(body or b"")} {token}\n'.encode()
            connection.write(packet); connection.flush()
            if body: connection.write(body); connection.flush()
            deadline=time.monotonic()+12; buffered=bytearray(); seen=0; last_kind='none'
            while time.monotonic()<deadline:
                buffered.extend(connection.read(max(1,min(4096,connection.in_waiting))))
                while b'\n' in buffered:
                    line,_,rest=buffered.partition(b'\n'); buffered=bytearray(rest)
                    seen+=len(line)+1
                    try: response=json.loads(line)
                    except (ValueError,UnicodeError): continue
                    if isinstance(response,dict) and response.get('kind')=='bridge' and response.get('id')==request_id:
                        return response['status'],json.dumps(response['body'],separators=(',',':')).encode()
                    if isinstance(response,dict): last_kind=str(response.get('kind','none'))[:32]
                if len(buffered)>16384: buffered.clear()
            print(f'USB response timeout id={request_id}, received={seen+len(buffered)}, last_kind={last_kind}',flush=True)
            raise TimeoutError('No complete response from ESP32')

    class Handler(BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'

        def forward(self):
            # Reject DNS rebinding and cross-origin control requests.
            if self.headers.get('Host') not in (f'127.0.0.1:{port}', f'localhost:{port}'):
                self.send_error(403); return
            origin = self.headers.get('Origin')
            if origin and origin not in (f'http://127.0.0.1:{port}', f'http://localhost:{port}'):
                self.send_error(403); return
            path = urlsplit(self.path)
            if path.scheme or path.netloc or path.query or path.fragment or not (
                path.path in ('/', '/simulation') or path.path.startswith('/api/')
            ):
                self.send_error(404); return
            if self.headers.get('Transfer-Encoding'):
                self.send_error(400); return
            try:
                size = int(self.headers.get('Content-Length', '0'))
                if not 0 <= size <= 131072:
                    self.send_error(413); return
            except ValueError:
                self.send_error(400); return
            self.connection.settimeout(3)
            try:
                body = self.rfile.read(size) if size else None
                if body is not None and len(body) != size:
                    self.send_error(400); return
                if connection is not None or preview_only:
                    if self.command=='GET' and path.path in ('/','/simulation'):
                        name='simulation_dashboard.h' if path.path=='/simulation' else 'dashboard.h'
                        source=(Path(__file__).resolve().parents[1]/'firmware/node_intelligence/src'/name).read_text(encoding='utf-8')
                        payload=source.split('R"HTML(',1)[1].split(')HTML"',1)[0].encode()
                        if preview_only:
                            notice=b'<aside role="status" style="padding:16px;background:#593e18;color:white;font:16px system-ui">OFFLINE LAYOUT PREVIEW: boards disconnected. Person and environment controls are synthetic inputs only. ESP32 inference, decisions and device outputs are unavailable.</aside>'
                            payload=payload.replace(b'<main>',notice+b'<main>',1)
                            payload=payload.replace(b'</html>',b'<script>document.getElementById("metrics")?.replaceChildren(document.createTextNode("No measurements: boards disconnected"));document.getElementById("speechStatus")?.replaceChildren(document.createTextNode("Speech unavailable until Node B reconnects"));</script></html>')
                        status=200; content_type='text/html; charset=utf-8'
                    elif preview_only:
                        status=503; payload=b'{"error":"Boards disconnected; interface preview only","known":false,"supported":false}'
                        content_type='application/json'
                    else:
                        status,payload=send_serial(self.command,path.path,body,self.headers.get('X-ClawGrid-Token'))
                        content_type='application/json'
                    self.send_response(status); self.send_header('Content-Type',content_type)
                    self.send_header('Content-Length',str(len(payload))); self.send_header('Cache-Control','no-store')
                    self.send_header('X-Content-Type-Options','nosniff')
                    self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; media-src blob:; frame-ancestors 'none'")
                    self.end_headers(); self.wfile.write(payload); return
                headers = {'Connection': 'close', 'Content-Type': self.headers.get('Content-Type', 'text/plain')}
                token = self.headers.get('X-ClawGrid-Token')
                if token: headers['X-ClawGrid-Token'] = token
                http_connection = http.client.HTTPConnection(board, timeout=5)
                try:
                    http_connection.request(self.command, path.path, body, headers)
                    response = http_connection.getresponse()
                    payload = response.read(262145)
                    if len(payload) > 262144: raise ValueError('Board response exceeds limit')
                    self.send_response(response.status)
                    self.send_header('Content-Type', response.getheader('Content-Type', 'application/json'))
                    self.send_header('Content-Length', str(len(payload)))
                    self.send_header('Cache-Control', 'no-store')
                    self.send_header('X-Content-Type-Options', 'nosniff')
                    self.send_header('Content-Security-Policy', "default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; media-src blob:; frame-ancestors 'none'")
                    self.end_headers(); self.wfile.write(payload)
                finally:
                    http_connection.close()
            except (OSError, ValueError, http.client.HTTPException) as error:
                print(f'Board transport failure: {self.command} {path.path}: {type(error).__name__}',flush=True)
                self.send_error(502, 'ESP32 unavailable; no simulated fallback')

        do_GET = forward
        do_POST = forward

        def log_message(self, fmt, *args):
            # Avoid request paths, headers, text or microphone data in logs.
            pass

    server = ThreadingHTTPServer(('127.0.0.1', port), Handler)
    server.daemon_threads = True
    print(f'Browser: http://127.0.0.1:{port}/simulation; '+('OFFLINE INTERFACE PREVIEW' if preview_only else f'ESP32 transport {serial_port or board}'), flush=True)
    try: server.serve_forever()
    except KeyboardInterrupt: pass
    finally:
        server.server_close()
        if connection is not None: connection.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--board', default='192.168.4.1')
    parser.add_argument('--port', type=int, default=8765)
    parser.add_argument('--serial',help='Use the board USB HTTP tunnel without changing laptop Wi-Fi')
    parser.add_argument('--preview-only',action='store_true',help='Show the interface without boards; all API calls explicitly return 503')
    args = parser.parse_args()
    if args.preview_only and args.serial: parser.error('--preview-only cannot access a serial board')
    run(args.board, args.port, args.serial,args.preview_only)
