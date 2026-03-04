const https = require('https');

let cachedAccessToken = null;
let tokenExpiresAt = 0;

function httpsRequest(options, postData) {
  return new Promise((resolve, reject) => {
    const req = https.request(options, (res) => {
      let data = '';
      res.on('data', (chunk) => { data += chunk; });
      res.on('end', () => {
        try {
          resolve({ statusCode: res.statusCode, body: JSON.parse(data) });
        } catch (e) {
          reject(new Error(`Invalid JSON response: ${data.substring(0, 200)}`));
        }
      });
    });
    req.on('error', reject);
    req.setTimeout(10000, () => {
      req.destroy(new Error('Request timeout'));
    });
    if (postData) req.write(postData);
    req.end();
  });
}

async function getOAuthAccessToken(accountId, clientId, clientSecret) {
  if (cachedAccessToken && Date.now() < tokenExpiresAt - 60000) {
    return cachedAccessToken;
  }

  const credentials = Buffer.from(`${clientId}:${clientSecret}`).toString('base64');
  const postData = `grant_type=account_credentials&account_id=${encodeURIComponent(accountId)}`;

  const result = await httpsRequest({
    hostname: 'zoom.us',
    path: '/oauth/token',
    method: 'POST',
    headers: {
      'Authorization': `Basic ${credentials}`,
      'Content-Type': 'application/x-www-form-urlencoded',
      'Content-Length': Buffer.byteLength(postData),
    },
  }, postData);

  if (result.statusCode !== 200 || !result.body.access_token) {
    throw new Error(`OAuth token request failed (${result.statusCode}): ${JSON.stringify(result.body)}`);
  }

  cachedAccessToken = result.body.access_token;
  tokenExpiresAt = Date.now() + (result.body.expires_in || 3600) * 1000;

  console.log('[RecordingToken] OAuth access token obtained');
  return cachedAccessToken;
}

async function getLocalRecordingToken(meetingId, accountId, clientId, clientSecret) {
  const cleanId = meetingId.replace(/[\s-]/g, '');

  const accessToken = await getOAuthAccessToken(accountId, clientId, clientSecret);

  const result = await httpsRequest({
    hostname: 'api.zoom.us',
    path: `/v2/meetings/${cleanId}/jointoken/local_recording`,
    method: 'GET',
    headers: {
      'Authorization': `Bearer ${accessToken}`,
      'Content-Type': 'application/json',
    },
  });

  if (result.statusCode !== 200 || !result.body.token) {
    throw new Error(`Local recording token request failed (${result.statusCode}): ${JSON.stringify(result.body)}`);
  }

  console.log('[RecordingToken] Local recording join token obtained for meeting', cleanId);
  return result.body.token;
}

module.exports = { getLocalRecordingToken };
