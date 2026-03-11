# ZoomLink SDI Server (Linux)

A standalone Linux server that receives video/audio streams from the ZoomLink Mac app and outputs them to Blackmagic DeckLink SDI cards.

## Architecture

```
Mac Mini (ZoomLink App)         TCP (port 9300)        Linux Server
┌──────────────────────┐       ─────────────────▶     ┌────────────────────┐
│ Joins Zoom meeting   │       Video frames           │ Receives frames    │
│ Captures ISO feeds   │       Audio buffers          │ Outputs to SDI     │
│ NDI + Recording      │       Participant info       │ Web dashboard      │
│ Sends to Linux       │                              │ REST API           │
└──────────────────────┘                              └────────────────────┘
```

## Requirements

- Ubuntu 22.04+ or similar Linux distribution
- Node.js 18+
- Blackmagic Desktop Video drivers and SDK (for DeckLink output)
- Network connectivity to the Mac running ZoomLink

## Quick Start

### 1. Install Blackmagic Desktop Video

Download from: https://www.blackmagicdesign.com/support/download/deb

```bash
sudo dpkg -i desktopvideo_*.deb
sudo apt-get install -f
```

Verify installation:
```bash
ls /usr/include/DeckLinkAPI.h
```

### 2. Run Setup

```bash
cd linux-server
bash install.sh
```

This will:
- Install Node.js if needed
- Install npm dependencies
- Build the DeckLink native addon
- Generate a systemd service file

### 3. Start the Server

```bash
node server.js
```

Or with custom ports:
```bash
TCP_PORT=9300 API_PORT=9301 node server.js
```

### 4. Access the Dashboard

Open http://YOUR_SERVER_IP:9301/ in a browser to see:
- Connection status
- DeckLink device list
- Participant list
- Output assignments

## Running as a System Service

After running `install.sh`:

```bash
sudo cp zoomlink-sdi.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable zoomlink-sdi
sudo systemctl start zoomlink-sdi
```

Check status:
```bash
sudo systemctl status zoomlink-sdi
journalctl -u zoomlink-sdi -f
```

## Firewall

Open these ports:

| Port | Protocol | Purpose |
|------|----------|---------|
| 9300 | TCP | Frame stream from Mac app |
| 9301 | TCP | HTTP API and web dashboard |

```bash
sudo ufw allow 9300/tcp
sudo ufw allow 9301/tcp
```

## REST API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | /api/status | Server status, clients, outputs |
| GET | /api/devices | List DeckLink devices and display modes |
| GET | /api/participants | List connected participants |
| POST | /api/output/start | Start output on a device (`{deviceIndex, mode}`) |
| POST | /api/output/stop | Stop output on a device (`{deviceIndex}`) |
| POST | /api/assign | Assign participant to device (`{userId, deviceIndex}`) |
| POST | /api/unassign | Remove assignment (`{userId}`) |

## Configuration

Environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| TCP_PORT | 9300 | Port for frame stream from Mac |
| API_PORT | 9301 | Port for HTTP API and dashboard |
| HOST | 0.0.0.0 | Bind address |

## Troubleshooting

### DeckLink not detected
- Verify drivers: `lsmod | grep blackmagic`
- Check devices: `BlackmagicFirmwareUpdater status` or check `/dev/blackmagic/`
- Ensure the card is seated properly in the PCIe slot

### No frames arriving
- Check Mac app is configured with the Linux server IP and port
- Verify firewall allows port 9300
- Check network connectivity: `nc -zv LINUX_IP 9300`

### Build errors for DeckLink addon
- Ensure build tools are installed: `sudo apt-get install build-essential python3`
- Ensure DeckLink SDK headers are in `/usr/include/`
- Check that `DeckLinkAPIDispatch.cpp` exists in `decklink-addon/src/`
