'use strict';

const http = require('http');
const path = require('path');
const fs = require('fs');
const { DISPLAY_MODES } = require('./decklink-manager');

function createApi(options = {}) {
  const { port = 9301, deckLinkManager, ndiManager, recorderManager, receiver } = options;

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
        ndi: ndiManager ? ndiManager.getStatus() : null,
        recording: recorderManager ? recorderManager.getStatus() : null,
      });
      return;
    }

    if (req.method === 'GET' && pathname === '/api/devices') {
      const devices = deckLinkManager ? deckLinkManager.getDevices() : [];
      const modes = {};
      for (const [key, val] of Object.entries(DISPLAY_MODES)) {
        modes[key] = val.label;
      }
      const outputStatus = {};
      if (deckLinkManager) {
        for (const [idx, out] of deckLinkManager.outputs) {
          outputStatus[idx] = { active: !!out.active, mode: out.modeKey || null };
        }
      }
      json(res, { devices, displayModes: modes, outputStatus });
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
      readBody(req, async (body) => {
        try {
          const { userId, deviceIndex } = body;
          if (userId === undefined || deviceIndex === undefined) {
            json(res, { success: false, error: 'userId and deviceIndex required' }, 400);
            return;
          }
          const result = await deckLinkManager.assignParticipant(userId, deviceIndex);
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

    if (req.method === 'POST' && pathname === '/api/ndi/create') {
      readBody(req, async (body) => {
        try {
          const { userId, displayName, isoIndex } = body;
          if (userId === undefined || !displayName) {
            json(res, { success: false, error: 'userId and displayName required' }, 400);
            return;
          }
          const source = await ndiManager.createSource(userId, displayName, isoIndex || 1);
          json(res, { success: !!source, source: source ? { sourceName: source.sourceName, mock: source.mock } : null });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/ndi/destroy') {
      readBody(req, (body) => {
        try {
          const { userId } = body;
          if (userId === undefined) {
            json(res, { success: false, error: 'userId required' }, 400);
            return;
          }
          ndiManager.destroySource(userId);
          json(res, { success: true });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/ndi/toggle') {
      readBody(req, (body) => {
        try {
          const { userId, active } = body;
          if (userId === undefined) {
            json(res, { success: false, error: 'userId required' }, 400);
            return;
          }
          ndiManager.toggleSource(userId, active !== false);
          json(res, { success: true });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/recording/start') {
      readBody(req, (body) => {
        try {
          const { userId, displayName } = body;
          if (userId === undefined) {
            json(res, { success: false, error: 'userId required' }, 400);
            return;
          }
          const name = displayName || (receiver.participants.get(userId) || {}).name || `User_${userId}`;
          const recorder = recorderManager.startRecording(userId, name);
          json(res, { success: !!recorder });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/recording/stop') {
      readBody(req, (body) => {
        try {
          const { userId } = body;
          if (userId === undefined) {
            json(res, { success: false, error: 'userId required' }, 400);
            return;
          }
          const recorder = recorderManager.stopRecording(userId);
          json(res, { success: !!recorder });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/recording/start-all') {
      readBody(req, (body) => {
        try {
          const participants = receiver ? receiver.participants : new Map();
          let started = 0;
          for (const [userId, p] of participants) {
            if (!recorderManager.isRecording(userId)) {
              const recorder = recorderManager.startRecording(userId, p.name || `User_${userId}`);
              if (recorder) started++;
            }
          }
          json(res, { success: true, started });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/recording/stop-all') {
      readBody(req, (body) => {
        try {
          recorderManager.stopAll();
          json(res, { success: true });
        } catch (err) {
          json(res, { success: false, error: err.message }, 500);
        }
      });
      return;
    }

    if (req.method === 'POST' && pathname === '/api/recording/set-dir') {
      readBody(req, (body) => {
        try {
          const { dir } = body;
          if (!dir) {
            json(res, { success: false, error: 'dir required' }, 400);
            return;
          }
          recorderManager.setOutputDir(dir);
          json(res, { success: true, outputDir: dir });
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
      return new Promise((resolve, reject) => {
        server.on('error', (err) => {
          console.error(`[API] Failed to start HTTP server on port ${port}:`, err.message);
          reject(err);
        });
        server.listen(port, '0.0.0.0', () => {
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
