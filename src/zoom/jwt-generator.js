const crypto = require('crypto');

function base64UrlEncode(data) {
  return Buffer.from(data)
    .toString('base64')
    .replace(/\+/g, '-')
    .replace(/\//g, '_')
    .replace(/=+$/, '');
}

function generateJWT(sdkKey, sdkSecret, sessionName, role = 1, sessionKey = '', userIdentity = '') {
  const iat = Math.floor(Date.now() / 1000) - 30;
  const exp = iat + 60 * 60 * 2;

  const header = {
    alg: 'HS256',
    typ: 'JWT',
  };

  const payload = {
    app_key: sdkKey,
    tpc: sessionName,
    role_type: role,
    version: 1,
    iat,
    exp,
    user_identity: userIdentity || `host_${Date.now()}`,
  };

  if (sessionKey) {
    payload.session_key = sessionKey;
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

module.exports = { generateJWT };
