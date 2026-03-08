# ZoomLink - Complete Setup Guide for macOS

This guide walks you through setting up ZoomLink on a brand new Mac, step by step. No prior experience needed.

---

## What You Need Before Starting

You need to download two things before running the setup:

### 1. Zoom Meeting SDK (Required)

This is the software that lets ZoomLink connect to Zoom meetings.

1. Go to https://marketplace.zoom.us and sign in with your Zoom account
2. Click "Build App" in the top menu
3. Create a new app and choose "Meeting SDK" as the app type
4. Fill in the required info (app name, description, etc.)
5. On the "Download" tab, download the **macOS** SDK (version 6.x)
6. Unzip the downloaded file — you'll get a folder like `zoom-sdk-macos-6.7.6.75900`
7. Move that folder somewhere you'll remember, like your Documents folder

**Also note down your SDK Key and SDK Secret** from the "App Credentials" section — you'll enter these in the app later.

### 2. NDI SDK (Handled Automatically)

The setup script downloads this for you. No action needed.

### 3. DeckLink SDK (Optional - Only for SDI Output)

Only needed if you have Blackmagic DeckLink cards for SDI output.

1. Go to https://www.blackmagicdesign.com/developer/product/decklink
2. Download the DeckLink SDK
3. Also install **Desktop Video** drivers from https://www.blackmagicdesign.com/support/family/capture-and-playback
4. Unzip the SDK somewhere in your Documents or Downloads folder

---

## Step-by-Step Setup

### Step 1: Download ZoomLink from Replit

1. In Replit, click the three dots menu (⋯) on the file tree
2. Choose "Download as ZIP"
3. On your Mac, unzip the downloaded file
4. Move the unzipped folder to a location you'll remember, like:
   `/Users/YourName/Documents/ZoomLink`

### Step 2: Open Terminal

1. Press **Command + Space** to open Spotlight
2. Type **Terminal** and press Enter
3. A window with a text prompt will appear — this is where you'll type commands

### Step 3: Navigate to the ZoomLink folder

Type this command (replace the path with where you put the folder):

```
cd /Users/YourName/Documents/ZoomLink
```

Press Enter.

### Step 4: Run the Setup Script

Type this command (replace the path with where you put the Zoom SDK):

```
bash scripts/setup-mac.sh /Users/YourName/Documents/zoom-sdk-macos-6.7.6.75900
```

Press Enter.

**What this does:**
- Installs Xcode Command Line Tools (if needed — a dialog may pop up, click "Install")
- Installs Homebrew (a package manager for Mac)
- Installs Node.js, Python, Git, and FFmpeg
- Sets up all the native addons (Zoom SDK, NDI, DeckLink)
- Links everything together

**This will take 10-20 minutes** on a fresh Mac. If it asks for your password, type your Mac login password (you won't see it as you type — that's normal) and press Enter.

If the script stops and says "re-run this script" (because Xcode tools needed to install), wait for the Xcode install to finish, then run the same command again.

### Step 5: Launch ZoomLink

The setup script automatically builds a `ZoomLink.app` in the `dist/` folder. You have two options:

**Option A: Double-click the app (recommended)**

1. Open Finder and navigate to the `dist` folder inside your ZoomLink project
2. You'll see `ZoomLink.app` — drag it to your **Applications** folder (or leave it in `dist/`)
3. The first time you open it: **right-click** the app and choose **Open** (this bypasses macOS security since the app isn't signed)
4. Click "Open" on the security dialog

**Option B: Run from Terminal**

```
cd /Users/YourName/Documents/ZoomLink
npm start
```

### Step 6: Enter Your Zoom Credentials

On first launch, a Settings dialog will automatically appear. Enter:

- **SDK Key** — from your Zoom Marketplace app
- **SDK Secret** — from your Zoom Marketplace app

The other fields (Account ID, Client ID, Client Secret) are optional — they enable automatic recording permission when joining meetings. You can add them later.

Click **Save**, then quit and relaunch the app.

### Step 7: Join a Meeting

1. Enter a Meeting ID and Passcode in the toolbar
2. Click "Join Meeting"
3. The bot will appear in the Zoom meeting as "ZoomLink"
4. The meeting host needs to grant recording permission when prompted
5. Once granted, ZoomLink will start capturing all participant feeds

---

## Running ZoomLink After Setup

Just double-click **ZoomLink.app** (from your Applications folder or the `dist/` folder).

That's it. The setup only needs to happen once.

If you prefer Terminal: `cd /Users/YourName/Documents/ZoomLink && npm start`

---

## What Each Feature Does

| Feature | What It Does | Requirements |
|---------|-------------|--------------|
| **NDI Output** | Sends each participant as a separate video source over your network (for OBS, vMix, Tricaster, etc.) | NDI SDK (installed automatically) |
| **Recording** | Records a separate MP4 file for each participant | FFmpeg (installed automatically) |
| **SDI Output** | Sends participants to physical SDI outputs on DeckLink cards | Blackmagic DeckLink card + drivers + SDK |

---

## Troubleshooting

### "npm: command not found"
Close Terminal and open a new one. The setup script added the necessary paths to your shell, but they only take effect in new Terminal windows.

### The app opens but shows "Credentials: Not configured"
Click the gear icon in the top-right corner and enter your SDK Key and SDK Secret.

### "Zoom SDK not found" during setup
Make sure you're pointing to the correct folder. The path should end with something like `zoom-sdk-macos-6.7.6.75900` and contain a `ZoomSDK` subfolder inside it.

### DeckLink not available
This is optional. If you don't have a Blackmagic DeckLink card, this is expected and everything else still works.

### NDI sources not showing up in other software
Make sure both machines are on the same network. In OBS/vMix/etc., look for sources named "ZoomLink - ParticipantName".

### Recording permission denied
The Zoom meeting host needs to allow recording. When ZoomLink joins, the host will see a popup asking to grant recording permission — they need to click "Allow".

---

## Updating ZoomLink

When you download a new version from Replit:

1. Download the ZIP from Replit
2. Replace the files in your ZoomLink folder (or unzip to a new location)
3. In Terminal, navigate to the folder and run:
   ```
   bash scripts/setup-mac.sh /path/to/zoom-sdk
   ```
   The script will skip anything already installed and only rebuild what changed.
4. Run `npm start`

Your settings (SDK credentials) are stored separately and will be preserved across updates.
