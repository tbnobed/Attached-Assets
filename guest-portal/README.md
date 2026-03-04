# Zoom ISO Capture - Guest Portal

A lightweight web page for remote guests to join your Zoom ISO Capture sessions via browser. No app install needed — guests open the link in Chrome or Edge and join.

## Setup on Linux Server

```bash
# 1. Copy the guest-portal folder to your server
scp -r guest-portal/ user@yourserver:/opt/guest-portal

# 2. SSH into your server
ssh user@yourserver

# 3. Navigate to the directory
cd /opt/guest-portal

# 4. Create your .env file
cp .env.example .env
nano .env
# Fill in your ZOOM_SDK_KEY and ZOOM_SDK_SECRET
# Set SESSION_NAME to match your Zoom ISO Capture desktop app session

# 5. Install dependencies
npm install

# 6. Start the server
npm start
```

## Configuration (.env)

| Variable | Required | Description |
|---|---|---|
| `ZOOM_SDK_KEY` | Yes | Your Zoom Video SDK key |
| `ZOOM_SDK_SECRET` | Yes | Your Zoom Video SDK secret |
| `SESSION_NAME` | No | Session name (default: PlexStudioSession) — must match the desktop app |
| `SESSION_PASSWORD` | No | Session password if set on the desktop app |
| `PORT` | No | HTTP port (default: 3000) |

## Usage

1. Start the guest portal on your server
2. Start a session on the Zoom ISO Capture desktop app (Windows)
3. Share `http://your-server-ip:3000` with your remote guests
4. Guests open the link, enter their name, and click "Join Session"
5. Their video/audio feeds appear in the desktop app as isolated NDI sources

## Production (Optional)

For HTTPS and production use, put the server behind nginx with a TLS certificate:

```nginx
server {
    listen 443 ssl;
    server_name guests.yourdomain.com;

    ssl_certificate /etc/letsencrypt/live/guests.yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/guests.yourdomain.com/privkey.pem;

    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

## Requirements

- Node.js 18+
- Guests need Chrome or Edge (Zoom Video SDK requirement)
- Camera and microphone access required for guests
- HTTPS required in production (camera access requires secure context)
