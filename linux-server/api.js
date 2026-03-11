'use strict';

const http = require('http');
const path = require('path');
const fs = require('fs');
const { DISPLAY_MODES } = require('./decklink-manager');

function createApi(options = {}) {
  const { port = 9301, deckLinkManager, receiver } = options;

  const server = http.createServer((req, res) => {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const pathname = url.pathname;

    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
      res.writeHead(204);
      res.end();
      return;
    }

    if (pathname === '/' || pathname === '/index.html') {
      serveFile(res, path.join(__dirname, 'web', 'index.html'), 'text/html');
      return;
    }

    if (pathname.startsWith('/web/')) {
      const webRoot = path.join(__dirname, 'web');
      const filePath = path.resolve(path.join(__dirname, pathname));
      if (!filePath.startsWith(webRoot + path.sep) && filePath !== webRoot) {
        res.writeHead(403);
        res.end('Forbidden');
        return;
      }
      const ext = path.extname(filePath);
      const types = { '.html': 'text/html', '.css': 'text/css', '.js': 'application/javascript', '.png': 'image/png' };
      serveFile(res, filePath, types[ext] || 'application/octet-stream');
      return;
    }

    if (req.method === 'GET' && pathname === '/api/status') {
      json(res, {
        server: { uptime: process.uptime(), version: '1.0.0' },
        receiver: receiver ? receiver.getStatus() : null,
        decklink: deckLinkManager ? deckLinkManager.getStatus() : null,
      });
      return;
    }

    if (req.method === 'GET' && pathname === '/api/devices') {
      const devices = deckLinkManager ? deckLinkManager.getDevices() : [];
      const modes = {};
      for (const [key, val] of Object.entries(DISPLAY_MODES)) {
        modes[key] = val.label;
      }
      json(res, { devices, displayModes: modes });
      return;
    }

    if (req.method === 'GET' && pathname === '/api/participants') {
      const participants = receiver ? Object.fromEntries(receiver.participants) : {};
      json(res, { participants });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/output/start') {
      readBody(req, async (body) => {
        try {
          const { deviceIndex, mode } = body;
          if (deviceIndex === undefined) {
            json(res, { success: false, error: 'deviceIndex required' }, 400);
            return;
          }
          const result = await deckLinkManager.startOutput(deviceIndex, mode || '1080p2997');
          json(res, result, result.success ? 200 : 400);
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/output/stop') {
      readBody(req, (body) => {
        try {
          const { deviceIndex } = body;
          if (deviceIndex === undefined) {
            json(res, { success: false, error: 'deviceIndex required' }, 400);
            return;
          }
          deckLinkManager.stopOutput(deviceIndex);
          json(res, { success: true });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/assign') {
      readBody(req, (body) => {
        try {
          const { userId, deviceIndex } = body;
          if (userId === undefined || deviceIndex === undefined) {
            json(res, { success: false, error: 'userId and deviceIndex required' }, 400);
            return;
          }
          const result = deckLinkManager.assignParticipant(userId, deviceIndex);
          json(res, result, result.success ? 200 : 400);
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/unassign') {
      readBody(req, (body) => {
        try {
          const { userId } = body;
          if (userId === undefined) {
            json(res, { success: false, error: 'userId required' }, 400);
            return;
          }
          deckLinkManager.unassignParticipant(userId);
          json(res, { success: true });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    res.writeHead(404);
    res.end(JSON.stringify({ error: 'Not found' }));
  });

  return {
    start() {
      return new Promise((resolve) => {
        server.listen(port, () => {
          console.log(`[API] HTTP API listening on port ${port}`);
          console.log(`[API] Dashboard: http://localhost:${port}/`);
          resolve();
        });
      });
    },
    stop() {
      server.close();
    },
  };
}

function json(res, data, status = 200) {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(data));
}

function readBody(req, callback) {
  let body = '';
  req.on('data', (chunk) => { body += chunk; });
  req.on('end', () => {
    try {
      callback(JSON.parse(body || '{}'));
    } catch (err) {
      callback({});
    }
  });
}

function serveFile(res, filePath, contentType) {
  try {
    const data = fs.readFileSync(filePath);
    res.writeHead(200, { 'Content-Type': contentType });
    res.end(data);
  } catch (err) {
    res.writeHead(404);
    res.end('Not found');
  }
}

module.exports = { createApi };
