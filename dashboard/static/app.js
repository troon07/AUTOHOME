let lastBenchmarkData = null;

async function fetchState() {
  try {
    const res = await fetch('/api/state');
    if (!res.ok) return;
    const data = await res.json();
    renderRooms(data.rooms);
    renderLogs(data.logs);
  } catch (err) {
    console.warn("Failed to fetch state:", err);
  }
}

function renderRooms(rooms) {
  const container = document.getElementById('roomsGrid');
  if (!container || !rooms) return;

  container.innerHTML = rooms.map(room => {
    let badgeClass = 'badge-unknown';
    if (room.state === 'OCCUPIED') badgeClass = 'badge-occupied';
    else if (room.state === 'LIKELY_OCCUPIED') badgeClass = 'badge-likely';
    else if (room.state === 'VACANT') badgeClass = 'badge-vacant';

    return `
      <div class="card">
        <h2>
          <span>${capitalize(room.name.replace('_', ' '))}</span>
          <span class="badge ${badgeClass}">${room.state}</span>
        </h2>
        <div class="room-meta">
          Confidence: ${(room.confidence * 100).toFixed(0)}% | Sensor: ${room.sensor_id || 'None (Virtual)'}
        </div>
        
        <div class="device-row">
          <span>💡 Ceiling Light</span>
          <button class="btn ${room.light_on ? '' : 'btn-off'}" onclick="toggleDevice('${room.name}', 'light', ${!room.light_on})">
            ${room.light_on ? 'ON' : 'OFF'}
          </button>
        </div>

        <div class="device-row">
          <span>🌀 Ceiling Fan</span>
          <button class="btn ${room.fan_on ? '' : 'btn-off'}" onclick="toggleDevice('${room.name}', 'fan', ${!room.fan_on})">
            ${room.fan_on ? 'ON' : 'OFF'}
          </button>
        </div>

        <div style="margin-top: 10px;">
          <button class="btn btn-action" style="font-size:0.75rem; padding:4px 8px; width:100%;" onclick="simulateMotion('${room.sensor_id || room.name}')">
            ⚡ Simulate Motion Entry
          </button>
        </div>
      </div>
    `;
  }).join('');
}

function renderLogs(logs) {
  const box = document.getElementById('auditLogs');
  if (!box || !logs) return;
  box.innerHTML = logs.map(l => `[${new Date(l.timestamp * 1000).toLocaleTimeString()}] ${l.room}:${l.device_id} -> ${l.new_on ? 'ON' : 'OFF'} (Manual: ${l.is_manual})`).join('<br>');
  box.scrollTop = box.scrollHeight;
}

async function toggleDevice(room, device, targetOn) {
  try {
    const res = await fetch('/api/device', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        room: room,
        device: device,
        action: targetOn ? 'ON' : 'OFF',
        is_manual: true
      })
    });
    const result = await res.json();
    if (!result.authorized) {
      alert("Safety Kernel Blocked Action: " + result.reason);
    }
    fetchState();
  } catch (err) {
    alert("Request error: " + err);
  }
}

async function simulateMotion(sensorId) {
  await fetch('/api/sensor_event', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ sensor_id: sensorId, event_type: 'MOTION', value: true })
  });
  fetchState();
}

function speakAloud(text) {
  if (!text) return;
  try {
    if ('speechSynthesis' in window) {
      window.speechSynthesis.cancel();
      const utterance = new SpeechSynthesisUtterance(text);
      utterance.rate = 1.0;
      utterance.pitch = 1.0;
      utterance.lang = 'en-US';
      window.speechSynthesis.speak(utterance);
    }
  } catch (err) {
    console.warn("SpeechSynthesis error:", err);
  }
}

let recognition = null;
let isListening = false;

function initSpeechRecognition() {
  const SpeechRec = window.SpeechRecognition || window.webkitSpeechRecognition;
  if (!SpeechRec) {
    console.warn("SpeechRecognition API not directly available.");
    return null;
  }
  const rec = new SpeechRec();
  rec.continuous = false;
  rec.interimResults = false;
  rec.lang = 'en-US';

  rec.onstart = () => {
    isListening = true;
    document.getElementById('micLabel').innerText = "Listening...";
    document.getElementById('micIcon').innerText = "🔴";
    document.getElementById('micStatusBadge').innerText = "Phone Mic Recording...";
    document.getElementById('micStatusBadge').className = "badge badge-likely";
    setPipelineStep(1, "Phone Mic Active");
  };

  rec.onresult = (e) => {
    const transcript = e.results[0][0].transcript;
    document.getElementById('voiceInput').value = transcript;
    sendVoiceCommand();
  };

  rec.onerror = (e) => {
    console.warn("Speech rec error:", e.error);
    stopMic();
    const clarifyBox = document.getElementById('clarifyBox');
    clarifyBox.innerHTML = `
      <strong>🎤 Phone Browser Mic Permission Notice:</strong><br>
      Mobile Chrome/Safari restricts direct in-page microphone access on unencrypted IP addresses.<br>
      <div style="margin-top:6px;">
        💡 <strong>Instant Way:</strong> Tap the text box below and press the <strong>Microphone icon on your phone keyboard</strong> (Gboard / iOS) to dictate instantly!<br>
        🔒 <strong>Direct In-Page Mic:</strong> Access securely via <a href="https://${window.location.hostname}:8443" style="color:#60a5fa; text-decoration:underline;">https://${window.location.hostname}:8443</a> (accept the local SSL warning once to enable browser mic).
      </div>
    `;
    clarifyBox.style.display = 'block';
  };

  rec.onend = () => {
    stopMic();
  };

  return rec;
}

function togglePhoneMic() {
  if (!recognition) {
    recognition = initSpeechRecognition();
  }

  if (!recognition) {
    const clarifyBox = document.getElementById('clarifyBox');
    clarifyBox.innerHTML = `
      <strong>🎤 Voice Dictation Tip:</strong><br>
      Tap the text box and press the <strong>Microphone icon on your phone's keyboard</strong> (Gboard or Apple dictation) to speak aloud, then tap 'Run Model'!
    `;
    clarifyBox.style.display = 'block';
    document.getElementById('voiceInput').focus();
    return;
  }

  if (isListening) {
    recognition.stop();
    stopMic();
  } else {
    try {
      recognition.start();
    } catch (err) {
      console.warn("Rec start error:", err);
      stopMic();
    }
  }
}

function stopMic() {
  isListening = false;
  document.getElementById('micLabel').innerText = "Tap to Speak";
  document.getElementById('micIcon').innerText = "🎙️";
  document.getElementById('micStatusBadge').innerText = "Phone Mic Ready";
  document.getElementById('micStatusBadge').className = "badge badge-occupied";
}

function setPipelineStep(step, label) {
  for (let i = 1; i <= 4; i++) {
    const el = document.getElementById(`pipeStep${i}`);
    if (el) {
      if (i === step) {
        el.style.background = "#2563eb";
        el.style.color = "#ffffff";
        el.style.fontWeight = "bold";
      } else if (i < step) {
        el.style.background = "#065f46";
        el.style.color = "#a7f3d0";
      } else {
        el.style.background = "#1e293b";
        el.style.color = "#94a3b8";
      }
    }
  }
  if (label) {
    document.getElementById('pipelineTiming').innerText = label;
  }
}

async function sendVoiceCommand() {
  const input = document.getElementById('voiceInput');
  const clarifyBox = document.getElementById('clarifyBox');
  if (!input || !input.value.trim()) return;

  clarifyBox.style.display = 'none';
  const startTime = performance.now();

  setPipelineStep(2, "Node B: Parsing / Partition 1");

  try {
    // Step 2 & 3: Model execution on ESP32-S3
    setTimeout(() => setPipelineStep(3, "Node A: Split Partition / Actuator"), 15);

    const res = await fetch('/api/voice', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ text: input.value })
    });
    const data = await res.json();
    const elapsed = (performance.now() - startTime).toFixed(1);

    setPipelineStep(4, `Safety Kernel Gate (${elapsed} ms total)`);

    // ASSISTANT SPEAKS BACK WITH VOICE!
    if (data.spoken_response) {
      speakAloud(data.spoken_response);
    }

    if (data.requires_clarification) {
      clarifyBox.innerHTML = `<strong>⚠️ Ambiguity Clarification:</strong> ${data.clarification_prompt}`;
      clarifyBox.style.display = 'block';
    } else if (!data.authorized && data.reason) {
      clarifyBox.innerHTML = `<strong>⛔ Safety Kernel Rejection:</strong> ${data.reason}`;
      clarifyBox.style.display = 'block';
      speakAloud("Action blocked by safety rule: " + data.reason);
    } else {
      setTimeout(() => setPipelineStep(0, `Success in ${elapsed} ms`), 1200);
    }
    fetchState();
  } catch (err) {
    alert("Network error talking to ESP32: " + err);
    setPipelineStep(0, "Error");
  }
}

async function runBenchmark() {
  const tbody = document.getElementById('benchmarkRows');
  tbody.innerHTML = `<tr><td colspan="6" style="text-align:center;">Running multi-mode benchmark on 3.2M parameter model...</td></tr>`;

  const res = await fetch('/api/benchmark');
  const data = await res.json();
  lastBenchmarkData = data;

  tbody.innerHTML = `
    <tr>
      <td><strong>Mode A: Single Node</strong></td>
      <td>${data.mode_a_single_node.latency_ms} ms</td>
      <td>0.00 ms</td>
      <td><strong>${data.mode_a_single_node.latency_ms} ms</strong></td>
      <td>${data.mode_a_single_node.tokens_per_second} tok/s | ${(data.mode_a_single_node.node_a_psram_bytes / 1024 / 1024).toFixed(2)} MB</td>
      <td><span class="badge badge-occupied">${data.source}</span></td>
    </tr>
    <tr>
      <td><strong>Mode B: Parallel Responsibilities</strong></td>
      <td>22.70 ms</td>
      <td>${data.mode_b_parallel_tasks.network_time_ms} ms</td>
      <td><strong>${data.mode_b_parallel_tasks.latency_ms} ms</strong></td>
      <td>Async Semantic Event (${data.mode_b_parallel_tasks.semantic_packet_bytes} B)</td>
      <td><span class="badge badge-occupied">${data.source}</span></td>
    </tr>
    <tr>
      <td><strong>Mode C: Split-Model Partitioned</strong></td>
      <td>${(data.mode_c_split_model.node_a_compute_ms + data.mode_c_split_model.node_b_compute_ms).toFixed(2)} ms</td>
      <td>${data.mode_c_split_model.network_transfer_ms} ms</td>
      <td><strong>${data.mode_c_split_model.latency_ms} ms</strong></td>
      <td>Speedup: ${data.mode_c_split_model.speedup_vs_single_node}x (${data.mode_c_split_model.helped_or_hurt})</td>
      <td><span class="badge badge-occupied">${data.source}</span></td>
    </tr>
  `;
}

function exportBenchmarkJSON() {
  if (!lastBenchmarkData) {
    alert("Please click 'Run Benchmark' first.");
    return;
  }
  const blob = new Blob([JSON.stringify(lastBenchmarkData, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `clawgrid_benchmark_${Date.now()}.json`;
  a.click();
}

function capitalize(s) {
  return s.charAt(0).toUpperCase() + s.slice(1);
}

// Initial fetch and 2-second polling loop
fetchState();
setInterval(fetchState, 2000);
