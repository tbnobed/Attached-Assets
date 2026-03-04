(function () {
  const DOM = {
    joinScreen: document.getElementById('join-screen'),
    sessionScreen: document.getElementById('session-screen'),
    joinForm: document.getElementById('join-form'),
    userName: document.getElementById('user-name'),
    sessionName: document.getElementById('session-name'),
    cameraSelect: document.getElementById('camera-select'),
    micSelect: document.getElementById('mic-select'),
    cameraPreview: document.getElementById('camera-preview'),
    previewPlaceholder: document.getElementById('preview-placeholder'),
    audioLevel: document.getElementById('audio-level'),
    joinBtn: document.getElementById('join-btn'),
    joinBtnText: document.getElementById('join-btn-text'),
    joinBtnLoading: document.getElementById('join-btn-loading'),
    errorBanner: document.getElementById('error-banner'),
    sessionLabel: document.getElementById('session-label'),
    participantCount: document.getElementById('participant-count'),
    videoGrid: document.getElementById('video-grid'),
    statusText: document.getElementById('status-text'),
    toggleCamera: document.getElementById('toggle-camera'),
    toggleMic: document.getElementById('toggle-mic'),
    leaveBtn: document.getElementById('leave-btn'),
  };

  let previewStream = null;
  let audioContext = null;
  let audioAnalyser = null;
  let animFrameId = null;
  let zoomClient = null;
  let zoomStream = null;
  let cameraOn = true;
  let micOn = true;
  let currentUserName = '';

  function showError(msg) {
    DOM.errorBanner.textContent = msg;
    DOM.errorBanner.classList.remove('hidden');
  }

  function hideError() {
    DOM.errorBanner.classList.add('hidden');
  }

  async function loadConfig() {
    try {
      const res = await fetch('/api/config');
      const config = await res.json();
      DOM.sessionName.value = config.sessionName;
      if (!config.hasCredentials) {
        showError('Server is missing Zoom SDK credentials. Contact the session host.');
        DOM.joinBtn.disabled = true;
      }
    } catch (err) {
      showError('Cannot reach server. Please refresh the page.');
    }
  }

  async function enumerateDevices() {
    try {
      await navigator.mediaDevices.getUserMedia({ video: true, audio: true });
      const devices = await navigator.mediaDevices.enumerateDevices();

      DOM.cameraSelect.innerHTML = '';
      DOM.micSelect.innerHTML = '';

      const cameras = devices.filter(d => d.kind === 'videoinput');
      const mics = devices.filter(d => d.kind === 'audioinput');

      if (cameras.length === 0) {
        DOM.cameraSelect.innerHTML = '<option value="">No camera found</option>';
      } else {
        cameras.forEach((d, i) => {
          const opt = document.createElement('option');
          opt.value = d.deviceId;
          opt.textContent = d.label || 'Camera ' + (i + 1);
          DOM.cameraSelect.appendChild(opt);
        });
      }

      if (mics.length === 0) {
        DOM.micSelect.innerHTML = '<option value="">No microphone found</option>';
      } else {
        mics.forEach((d, i) => {
          const opt = document.createElement('option');
          opt.value = d.deviceId;
          opt.textContent = d.label || 'Microphone ' + (i + 1);
          DOM.micSelect.appendChild(opt);
        });
      }

      DOM.joinBtn.disabled = false;
      startPreview();
    } catch (err) {
      showError('Camera/microphone access denied. Please allow access and refresh.');
      DOM.cameraSelect.innerHTML = '<option value="">Permission denied</option>';
      DOM.micSelect.innerHTML = '<option value="">Permission denied</option>';
    }
  }

  async function startPreview() {
    stopPreview();
    const cameraId = DOM.cameraSelect.value;
    const micId = DOM.micSelect.value;
    if (!cameraId && !micId) return;

    try {
      const constraints = {};
      if (cameraId) constraints.video = { deviceId: { exact: cameraId }, width: 1280, height: 720 };
      if (micId) constraints.audio = { deviceId: { exact: micId } };

      previewStream = await navigator.mediaDevices.getUserMedia(constraints);

      if (cameraId) {
        DOM.cameraPreview.srcObject = previewStream;
        DOM.previewPlaceholder.classList.add('hidden');
      }

      if (micId) {
        audioContext = new AudioContext();
        const source = audioContext.createMediaStreamSource(previewStream);
        audioAnalyser = audioContext.createAnalyser();
        audioAnalyser.fftSize = 256;
        source.connect(audioAnalyser);
        animateAudioMeter();
      }
    } catch (err) {
      console.warn('Preview error:', err);
    }
  }

  function stopPreview() {
    if (previewStream) {
      previewStream.getTracks().forEach(t => t.stop());
      previewStream = null;
    }
    if (audioContext) {
      audioContext.close().catch(() => {});
      audioContext = null;
      audioAnalyser = null;
    }
    if (animFrameId) {
      cancelAnimationFrame(animFrameId);
      animFrameId = null;
    }
    DOM.cameraPreview.srcObject = null;
  }

  function animateAudioMeter() {
    if (!audioAnalyser) return;
    const data = new Uint8Array(audioAnalyser.frequencyBinCount);
    function update() {
      audioAnalyser.getByteFrequencyData(data);
      let sum = 0;
      for (let i = 0; i < data.length; i++) sum += data[i];
      const avg = sum / data.length;
      const pct = Math.min(100, (avg / 128) * 100);
      DOM.audioLevel.style.width = pct + '%';
      animFrameId = requestAnimationFrame(update);
    }
    update();
  }

  DOM.cameraSelect.addEventListener('change', startPreview);
  DOM.micSelect.addEventListener('change', startPreview);

  DOM.userName.addEventListener('input', () => {
    hideError();
  });

  DOM.joinForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    hideError();

    const name = DOM.userName.value.trim();
    if (!name) {
      showError('Please enter your name.');
      return;
    }

    DOM.joinBtn.disabled = true;
    DOM.joinBtnText.classList.add('hidden');
    DOM.joinBtnLoading.classList.remove('hidden');

    try {
      const res = await fetch('/api/token', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          userName: name,
        }),
      });

      const data = await res.json();
      if (!res.ok) throw new Error(data.error || 'Failed to get token');

      currentUserName = data.userName;
      await joinSession(data);
    } catch (err) {
      showError(err.message);
      DOM.joinBtn.disabled = false;
      DOM.joinBtnText.classList.remove('hidden');
      DOM.joinBtnLoading.classList.add('hidden');
    }
  });

  async function joinSession(tokenData) {
    if (typeof ZoomVideo === 'undefined') {
      showError('Zoom Video SDK not loaded. Please refresh the page.');
      DOM.joinBtn.disabled = false;
      DOM.joinBtnText.classList.remove('hidden');
      DOM.joinBtnLoading.classList.add('hidden');
      return;
    }

    try {
      zoomClient = ZoomVideo.createClient();
      await zoomClient.init('en-US', 'Global', { patchJsMedia: true });

      await zoomClient.join(
        tokenData.sessionName,
        tokenData.token,
        tokenData.userName,
        ''
      );

      zoomStream = zoomClient.getMediaStream();

      stopPreview();

      const cameraId = DOM.cameraSelect.value;
      const micId = DOM.micSelect.value;

      if (cameraId) {
        await zoomStream.startVideo({ cameraId, virtualBackground: false });
        cameraOn = true;
      }

      if (micId) {
        await zoomStream.startAudio({ microphoneId: micId });
        micOn = true;
      }

      showSessionScreen(tokenData.sessionName);
      setupSessionEvents();
      renderParticipants();
    } catch (err) {
      console.error('Join error:', err);
      showError('Failed to join session: ' + (err.reason || err.message || 'Unknown error'));
      DOM.joinBtn.disabled = false;
      DOM.joinBtnText.classList.remove('hidden');
      DOM.joinBtnLoading.classList.add('hidden');
    }
  }

  function showSessionScreen(sessionName) {
    DOM.joinScreen.classList.remove('active');
    DOM.sessionScreen.classList.add('active');
    DOM.sessionLabel.textContent = sessionName;
  }

  function showJoinScreen() {
    DOM.sessionScreen.classList.remove('active');
    DOM.joinScreen.classList.add('active');
    DOM.joinBtn.disabled = false;
    DOM.joinBtnText.classList.remove('hidden');
    DOM.joinBtnLoading.classList.add('hidden');
  }

  function setupSessionEvents() {
    zoomClient.on('user-added', () => renderParticipants());
    zoomClient.on('user-removed', () => renderParticipants());
    zoomClient.on('user-updated', () => renderParticipants());
    zoomClient.on('video-active-change', () => renderParticipants());

    zoomClient.on('connection-change', (payload) => {
      if (payload.state === 'Closed' || payload.state === 'Fail') {
        DOM.statusText.textContent = 'Disconnected';
        setTimeout(() => {
          cleanup();
          showJoinScreen();
          showError('Session ended or connection lost.');
        }, 1000);
      }
    });
  }

  function renderParticipants() {
    if (!zoomClient) return;

    const participants = zoomClient.getAllUser();
    const count = participants.length;
    DOM.participantCount.textContent = count + ' participant' + (count !== 1 ? 's' : '');

    DOM.videoGrid.className = 'video-grid';
    if (count >= 5) DOM.videoGrid.classList.add('grid-6');
    else if (count >= 3) DOM.videoGrid.classList.add('grid-4');
    else if (count === 2) DOM.videoGrid.classList.add('grid-2');

    DOM.videoGrid.innerHTML = '';

    participants.forEach(user => {
      const cell = document.createElement('div');
      cell.className = 'video-cell';
      cell.id = 'video-cell-' + user.userId;

      const nameTag = document.createElement('div');
      nameTag.className = 'name-tag';
      nameTag.textContent = user.displayName || 'Guest';

      if (user.bVideoOn) {
        const canvas = document.createElement('canvas');
        canvas.width = 1280;
        canvas.height = 720;
        canvas.style.width = '100%';
        canvas.style.height = '100%';
        canvas.style.objectFit = 'cover';

        cell.appendChild(canvas);
        cell.appendChild(nameTag);
        DOM.videoGrid.appendChild(cell);

        try {
          zoomStream.renderVideo(canvas, user.userId, 1280, 720, 0, 0, 2);
        } catch (err) {
          console.warn('Render video error for', user.displayName, err);
        }
      } else {
        const avatar = document.createElement('div');
        avatar.className = 'avatar';
        avatar.textContent = (user.displayName || 'G').charAt(0).toUpperCase();
        cell.appendChild(avatar);
        cell.appendChild(nameTag);
        DOM.videoGrid.appendChild(cell);
      }
    });

    DOM.statusText.textContent = 'Connected';
  }

  DOM.toggleCamera.addEventListener('click', async () => {
    if (!zoomStream) return;
    try {
      if (cameraOn) {
        await zoomStream.stopVideo();
        cameraOn = false;
        DOM.toggleCamera.classList.remove('active');
        DOM.toggleCamera.classList.add('muted');
      } else {
        await zoomStream.startVideo();
        cameraOn = true;
        DOM.toggleCamera.classList.add('active');
        DOM.toggleCamera.classList.remove('muted');
      }
      renderParticipants();
    } catch (err) {
      console.warn('Toggle camera error:', err);
    }
  });

  DOM.toggleMic.addEventListener('click', async () => {
    if (!zoomStream) return;
    try {
      if (micOn) {
        await zoomStream.muteAudio();
        micOn = false;
        DOM.toggleMic.classList.remove('active');
        DOM.toggleMic.classList.add('muted');
      } else {
        await zoomStream.unmuteAudio();
        micOn = true;
        DOM.toggleMic.classList.add('active');
        DOM.toggleMic.classList.remove('muted');
      }
    } catch (err) {
      console.warn('Toggle mic error:', err);
    }
  });

  DOM.leaveBtn.addEventListener('click', async () => {
    try {
      if (zoomClient) await zoomClient.leave();
    } catch (err) {
      console.warn('Leave error:', err);
    }
    cleanup();
    showJoinScreen();
  });

  function cleanup() {
    zoomStream = null;
    if (zoomClient) {
      try { zoomClient.leave(); } catch (e) {}
      zoomClient = null;
    }
    cameraOn = true;
    micOn = true;
    DOM.toggleCamera.classList.add('active');
    DOM.toggleCamera.classList.remove('muted');
    DOM.toggleMic.classList.add('active');
    DOM.toggleMic.classList.remove('muted');
  }

  loadConfig();
  enumerateDevices();
})();
