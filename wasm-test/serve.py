#!/usr/bin/env python3
"""Serve the wasm-test directory with correct MIME types for WASM/data files."""
import http.server
import sys
import os

MIME_OVERRIDES = {
    '.wasm': 'application/wasm',
    '.data': 'application/octet-stream',
    '.js':   'application/javascript',
}

class WasmHandler(http.server.SimpleHTTPRequestHandler):
    def guess_type(self, path):
        ext = os.path.splitext(path)[1].lower()
        if ext in MIME_OVERRIDES:
            return MIME_OVERRIDES[ext]
        return super().guess_type(path)

    def end_headers(self):
        # Required for SharedArrayBuffer / COOP+COEP if pthreads are ever enabled
        # self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        # self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

    def log_message(self, fmt, *args):
        print(fmt % args)


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    directory = sys.argv[2] if len(sys.argv) > 2 else '.'
    abs_directory = os.path.abspath(directory)
    os.chdir(abs_directory)
    print(f'Serving {abs_directory} on http://localhost:{port}/')
    print('Open http://localhost:{}/test.html in your browser.'.format(port))
    http.server.test(HandlerClass=WasmHandler, port=port, bind='localhost')
