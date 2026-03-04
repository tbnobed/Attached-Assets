const express = require('express');
const path = require('path');
const crypto = require('crypto');
const fs = require('fs');

const app = express();

function loadEnv() {
  const envPath = path.join(__dirname, '.env');
  if (fs.existsSync(envPath)) {
    const content = fs.readFileSync(envPath, 'utf8');
    content.split('\n').forEach(line => {
      const trimmed = line.trim();
      if (trimmed && !trimmed.startsWith('#')) {
        const eqIndex = trimmed.indexOf('=');
        if (eqIndex > 0) {
          const key = trimmed.substring(0, eqIndex).trim();
          const value = trimmed.substring(eqIndex + 1).trim();
          if (!process.env[key]) {
            process.env[key] = value;
          }
        }
      }
    });
  }
}

loadEnv();

const SDK_KEY = process.env.ZOOM_SDK_KEY || '';
const SDK_SECRET = process.env.ZOOM_SDK_SECRET || '';
const DEFAULT_SESSION = process.env.SESSION_NAME || 'PlexStudioSession';
const SESSION_PASSWORD = process.env.SESSION_PASSWORD || '';
const PORT = parseInt(process.env.PORT, 10) || 3000;

function base64UrlEncode(data) {
  return Buffer.from(data)
    .toString('base64')
    .replace(/\+/g, '-')
    .replace(/\//g, '_')
    .replace(/=+$/, '');
}

function generateJWT(sdkKey, sdkSecret, sessionName, role, userIdentity) {
  const iat = Math.floor(Date.now() / 1000) - 30;
  const exp = iat + 60 * 60 * 2;

  const header = { alg: 'HS256', typ: 'JWT' };
  const payload = {
    app_key: sdkKey,
    tpc: sessionName,
    role_type: role,
    version: 1,
    iat,
    exp,
    user_identity: userIdentity,
  };

  if (SESSION_PASSWORD) {
    payload.session_key = SESSION_PASSWORD;
  }

  const encodedHeader = base64UrlEncode(JSON.stringify(header));
  const encodedPayload = base64UrlEncode(JSON.stringify(payload));
  const signingInput = `${encodedHeader}.${encodedPayload}`;

  const signature = crypto
    .createHmac('sha256', sdkSecret)
    .update(signingInput)
    .digest('base64')
    .replace(/\+/g, '-')
    .replace(/\//g, '_')
    .replace(/=+$/, '');

  return `${signingInput}.${signature}`;
}

app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

app.get('/api/config', (req, res) => {
  res.json({
    sessionName: DEFAULT_SESSION,
    hasCredentials: !!(SDK_KEY && SDK_SECRET),
  });
});

app.post('/api/token', (req, res) => {
  const { userName } = req.body;

  if (!SDK_KEY || !SDK_SECRET) {
    return res.status(500).json({ error: 'Zoom SDK credentials not configured on the server.' });
  }

  if (!userName || !userName.trim()) {
    return res.status(400).json({ error: 'Name is required.' });
  }

  const identity = userName.trim().replace(/[^a-zA-Z0-9_\- ]/g, '');

  try {
    const token = generateJWT(SDK_KEY, SDK_SECRET, DEFAULT_SESSION, 0, identity);
    res.json({
      token,
      sdkKey: SDK_KEY,
      sessionName: DEFAULT_SESSION,
      userName: identity,
    });
  } catch (err) {
    console.error('Token generation error:', err);
    res.status(500).json({ error: 'Failed to generate session token.' });
  }
});

app.listen(PORT, '0.0.0.0', () => {
  console.log('');
  console.log('  Zoom ISO - Guest Portal');
  console.log('  -----------------------');
  console.log('  URL:       http://localhost:' + PORT);
  console.log('  Session:   ' + DEFAULT_SESSION);
  console.log('  SDK ready: ' + (!!(SDK_KEY && SDK_SECRET)));
  console.log('');
  console.log('  Share the URL with your remote guests.');
  console.log('  They open it in Chrome/Edge, enter their name, and join.');
  console.log('');
});
