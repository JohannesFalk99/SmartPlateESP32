document.addEventListener('DOMContentLoaded', () => {
  // --- Notepad Logic ---
  function getNotepadKey() {
    const exp = document.getElementById('notepadExperiment').value.trim();
    return exp ? `notepad_${exp}` : 'notepad_default';
  }

  function showNotepadStatus(msg, isError = false) {
    const el = document.getElementById('notepadStatus');
    el.textContent = msg;
    el.style.color = isError ? 'red' : 'limegreen';
    setTimeout(() => { el.textContent = ''; }, 2000);
  }

  async function saveNotepad() {
    const key = getNotepadKey();
    const notes = document.getElementById('notepadArea').value;
    localStorage.setItem(key, notes);
    // Backend save
    try {
      const exp = document.getElementById('notepadExperiment').value.trim();
      const res = await fetch('/api/notepad', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ experiment: exp, notes })
      });
      if (!res.ok) throw new Error('Backend save failed');
      showNotepadStatus('Saved!');
    } catch (e) {
      showNotepadStatus('Saved locally, but backend failed', true);
    }
  }

  async function loadNotepad() {
    const key = getNotepadKey();
    // Try backend first
    const exp = document.getElementById('notepadExperiment').value.trim();
    if (exp) {
      try {
        const res = await fetch(`/api/notepad?experiment=${encodeURIComponent(exp)}`);
        if (res.ok) {
          const data = await res.json();
          document.getElementById('notepadArea').value = data.notes || '';
          showNotepadStatus('Loaded from backend');
          return;
        }
      } catch {}
    }
    // Fallback to localStorage
    const notes = localStorage.getItem(key) || '';
    document.getElementById('notepadArea').value = notes;
    showNotepadStatus('Loaded from local storage');
  }

  function clearNotepad() {
    document.getElementById('notepadArea').value = '';
    showNotepadStatus('Cleared');
  }

  // Attach event listeners
  document.getElementById('notepadSave')?.addEventListener('click', saveNotepad);
  document.getElementById('notepadLoad')?.addEventListener('click', loadNotepad);
  document.getElementById('notepadClear')?.addEventListener('click', clearNotepad);

  // Optional: auto-load when switching to Notepad tab
  $("a[href='#notepadTab']").on('click', loadNotepad);

  // --- End Notepad Logic ---
  let chartStartTime = null;
  let autoScroll = true;
  let fullData = { labels: [], temps: [] };
  let prevState = {};

  const ctx = document.getElementById('tempChart').getContext('2d');

  const tempData = {
    labels: [],
    datasets: [
      {
        label: 'Temperature (°C)',
        type: 'line',
        data: [],
        backgroundColor: 'rgba(255,99,132,0.4)',
        borderColor: 'rgba(255,99,132,1)',
        fill: true,
        tension: 0.4,
        pointRadius: 3,
        pointHoverRadius: 6
      },
      {
        label: 'Events',
        type: 'scatter',
        data: [],
        backgroundColor: 'yellow',
        borderColor: 'black',
        pointStyle: 'triangle',
        pointRadius: 6,
        pointHoverRadius: 8,
        showLine: false
      }
    ]
  };

  const tempChart = new Chart(ctx, {
    type: 'line',
    data: tempData,
    options: {
      responsive: true,
      animation: { duration: 800, easing: 'easeOutQuart' },
      scales: {
        x: {
          type: 'linear',
          title: { display: true, text: 'Time (s or min)' },
          ticks: { color: 'white' }
        },
        y: {
          beginAtZero: true,
          title: { display: true, text: '°C' },
          ticks: { color: 'white' }
        }
      },
      plugins: {
        legend: { labels: { color: 'white' } },
        tooltip: {
          callbacks: {
            label: (ctx) => {
              if (ctx.dataset.label === 'Events') return ctx.raw.description;
              return `Temp: ${ctx.raw.y.toFixed(2)} °C`;
            }
          }
        }
      }
    }
  });

  function formatTime(seconds) {
    return seconds < 60 ? `${seconds}s` : `${Math.floor(seconds / 60)}m`;
  }

  function getTemperatureAt(timeSec) {
    const index = fullData.labels.findIndex(t => t === timeSec);
    if (index !== -1) return fullData.temps[index];

    // fallback to nearest
    let closestIdx = 0;
    let minDiff = Infinity;
    for (let i = 0; i < fullData.labels.length; i++) {
      const diff = Math.abs(fullData.labels[i] - timeSec);
      if (diff < minDiff) {
        minDiff = diff;
        closestIdx = i;
      }
    }
    return fullData.temps[closestIdx];
  }

  function addEvent(timeSec, description) {
    const y = getTemperatureAt(timeSec) + 2;
    tempData.datasets[1].data.push({ x: timeSec, y, description });
  }

  function renderEvents(events) {
    console.log('Rendering events:', events);  // <-- Log events JSON here
    const addedTimes = new Set(tempData.datasets[1].data.map(e => e.x));
    events.forEach(event => {
      const eventTime = Math.floor((new Date(event.time).getTime() - chartStartTime) / 1000);
      if (!addedTimes.has(eventTime)) {
        const y = getTemperatureAt(eventTime) + 2;
        tempData.datasets[1].data.push({ x: eventTime, y, description: event.description });
      }
    });
  }

  function updateChartAggregated() {
    tempData.labels = fullData.labels;
    tempData.datasets[0].data = fullData.labels.map((x, i) => ({ x, y: fullData.temps[i] }));
    const maxY = Math.max(...tempData.datasets[0].data.map(d => d.y), ...tempData.datasets[1].data.map(d => d.y || 0), 50);
    tempChart.options.scales.y.max = Math.ceil((maxY + 10) / 5) * 5;
    tempChart.update();
  }

  function updateChart(data) {
    console.log('Updating chart with data:', data);  // <-- Log dataUpdate JSON here
    if (!chartStartTime) chartStartTime = Date.now();
    const elapsedSec = Math.floor((Date.now() - chartStartTime) / 1000);
    fullData.labels.push(elapsedSec);
    fullData.temps.push(data.temperature);

    if (prevState.temp_setpoint !== undefined) {
      if (data.temp_setpoint !== prevState.temp_setpoint)
        addEvent(elapsedSec, `Temp setpoint changed to ${data.temp_setpoint}°C`);
      if (data.rpm_setpoint !== prevState.rpm_setpoint)
        addEvent(elapsedSec, `RPM changed to ${data.rpm_setpoint}`);
      if (data.mode !== prevState.mode)
        addEvent(elapsedSec, `Mode changed to ${data.mode}`);
    }

    prevState = {
      temp_setpoint: data.temp_setpoint,
      rpm_setpoint: data.rpm_setpoint,
      mode: data.mode
    };

    updateChartAggregated();
  }

  function updateInfoBoxes(data) {
    console.log('Updating info boxes with data:', data);  // <-- Log dataUpdate JSON here
    $('#temp').text(`${data.temperature.toFixed(1)} °C`);
    $('#rpm').text(data.rpm);
    $('#mode').text(data.mode);
    const rt = data.running_time;
    $('#runningTime').text(rt < 60 ? `${rt}s` : `${Math.floor(rt / 60)}m ${Math.floor(rt % 60)}s`);
  }

  function sendControlUpdate(updateData) {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ action: 'controlUpdate', data: updateData }));
    }
  }

  // WebSocket setup
  const ws = new WebSocket(`ws://${location.host}/ws`);

  ws.onopen = () => {
    console.log('WebSocket connected');
    ws.send(JSON.stringify({ action: 'getHistory' }));
  };

  ws.onmessage = (event) => {
    try {
      console.log('WebSocket message received:', event.data);  // <-- Log raw WebSocket data here
      const msg = JSON.parse(event.data);
      if (msg.type === 'history') {
        console.log('History data received:', msg.data);  // <-- Log parsed history data
        if (!msg.data.length) {
          chartStartTime = Date.now();
        } else {
          chartStartTime = new Date(msg.data[0].time).getTime();
          fullData.labels = [];
          fullData.temps = [];
          tempData.datasets[1].data = [];
          msg.data.forEach(point => {
            const elapsedSec = Math.floor((new Date(point.time).getTime() - chartStartTime) / 1000);
            fullData.labels.push(elapsedSec);
            fullData.temps.push(point.temperature);
          });
          updateChartAggregated();
        }
      } else if (msg.type === 'dataUpdate') {
        updateInfoBoxes(msg.data);
        updateChart(msg.data);
        if (!window.formSyncedOnce) syncForm(msg.data);
      } else if (msg.type === 'events') {
        renderEvents(msg.data);
        updateChartAggregated();
      }
    } catch (e) {
      console.error('WebSocket message error:', e);
    }
  };

  ws.onclose = () => console.log('WebSocket disconnected');
  ws.onerror = (error) => console.error('WebSocket error:', error);

  // Event handlers
  $('#modeSelect').on('change', updateModeFields);
  $('#controlForm').on('submit', (e) => {
    e.preventDefault();
    const payload = {
      action: 'controlUpdate',
      data: {
        temp_setpoint: parseFloat($('#tempSetpoint').val()) || undefined,
        rpm_setpoint: parseInt($('#rpmSetpoint').val()) || undefined,
        mode: $('#modeSelect').val()
      }
    };
    if (payload.data.mode === 'Ramp') payload.data.ramp_rate = parseFloat($('#rampRate').val()) || undefined;
    if (payload.data.mode === 'Timer') payload.data.duration = parseInt($('#timerDuration').val()) || undefined;
    sendControlUpdate(payload.data);
    alert('Settings updated');
    window.formSyncedOnce = false;
  });

  function syncForm(data) {
    $('#tempSetpoint').val(data.temp_setpoint ?? '');
    $('#rpmSetpoint').val(data.rpm_setpoint ?? '');
    $('#modeSelect').val(data.mode ?? 'Off');
    $('#rampRate').val(data.ramp_rate ?? '');
    $('#timerDuration').val(data.duration ?? '');
    $('#alertRpmThreshold').val(data.alertRpmThreshold ?? '');
    $('#alertTimerThreshold').val(data.alertTimerThreshold ?? '');
    updateModeFields();
    window.formSyncedOnce = true;
  }

  function updateModeFields() {
    const mode = $('#modeSelect').val();
    $('.mode-field').hide();
    if (mode === 'Ramp') $('#rampRateGroup').show();
    else if (mode === 'Timer') $('#timerDurationGroup').show();
  }

  // Inline control update for special fields
  document.getElementById("alertRpmThreshold").addEventListener("change", (e) => {
    sendControlUpdate({ alertRpmThreshold: parseFloat(e.target.value) });
  });

  document.getElementById("alertTimerThreshold").addEventListener("change", (e) => {
    sendControlUpdate({ alertTimerThreshold: parseFloat(e.target.value) });
  });

  // Pause scrolling when hovering chart
  document.getElementById('tempChart').addEventListener('mouseenter', () => autoScroll = false);
  document.getElementById('tempChart').addEventListener('mouseleave', () => autoScroll = true);
});
