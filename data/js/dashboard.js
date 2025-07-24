document.addEventListener('DOMContentLoaded', () => {
  let ws;
  let chartStartTime = null;
  let autoScroll = true;
  let fullData = { labels: [], temps: [] };
  let prevState = {};
  let currentMode = null;
  let modeStartTime = null;
  let tempChart;

  // ---------------- WebSocket ----------------

  function initWebSocket() {
    ws = new WebSocket(`ws://${location.host}/ws`);

    ws.onopen = () => {
      console.log('WebSocket connected');
      ws.send(JSON.stringify({ action: 'getHistory' }));
      ws.send(JSON.stringify({ action: 'notepadList' }));
    };

    ws.onmessage = handleWebSocketMessage;
    ws.onclose = () => console.log('WebSocket disconnected');
    ws.onerror = (error) => console.error('WebSocket error:', error);
  }

  function sendMessage(payload) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(payload));
    } else {
      alert('WebSocket not connected.');
    }
  }

  // ---------------- Notepad Logic ----------------

  function setupNotepadHandlers() {
    document.getElementById('notepadSave').onclick = () => {
      const name = document.getElementById('notepadName').value.trim();
      const notes = document.getElementById('notepadArea').value;
      if (!name) return alert('Please enter a note name.');
      sendMessage({ action: 'notepadSave', experiment: name, notes: notes });
    };

    document.getElementById('notepadClear').onclick = () => {
      document.getElementById('notepadArea').value = '';
    };

    document.getElementById('notepadRefreshList').onclick = refreshNotepadList;

    document.getElementById('notepadList').onchange = () => {
      const name = document.getElementById('notepadList').value;
      if (!name) return;
      sendMessage({ action: 'notepadLoad', experiment: name });
    };
  }

  function refreshNotepadList() {
    sendMessage({ action: 'notepadList' });
  }

  function populateNotepadList(experiments) {
    const list = document.getElementById('notepadList');
    list.innerHTML = '';
    experiments.forEach(name => {
      const opt = document.createElement('option');
      opt.value = name;
      opt.textContent = name;
      list.appendChild(opt);
    });
  }

  // ---------------- Chart Setup ----------------

  function initChart() {
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

    tempChart = new Chart(ctx, {
      type: 'line',
      data: tempData,
      options: {
        responsive: true,
        animation: { duration: 300, easing: 'linear' },
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
              label: (ctx) =>
                ctx.dataset.label === 'Events'
                  ? ctx.raw.description
                  : `Temp: ${ctx.raw.y.toFixed(2)} °C`
            }
          }
        }
      }
    });

    document.getElementById('tempChart').addEventListener('mouseenter', () => autoScroll = false);
    document.getElementById('tempChart').addEventListener('mouseleave', () => autoScroll = true);
  }

  function updateChart(data) {
    // Ensure chartStartTime is initialized only once
    if (!chartStartTime) {
      console.warn('chartStartTime missing, using current time as fallback.');
      chartStartTime = Date.now(); // Only fallback if truly uninitialized
      return; // Skip this update to avoid bad elapsedSec
    }

    const elapsedSec = Math.floor((Date.now() - chartStartTime) / 1000);
    if (elapsedSec < 0 || isNaN(elapsedSec)) return; // Avoid corrupt points

    fullData.labels.push(elapsedSec);
    fullData.temps.push(data.temperature);

    detectAndAddEvents(data, elapsedSec);
    updateChartAggregated();
  }
  

  function detectAndAddEvents(data, elapsedSec) {
    if (prevState.temp_setpoint !== undefined) {
      if (data.temp_setpoint !== prevState.temp_setpoint)
        addEvent(elapsedSec, `Temp setpoint changed to ${data.temp_setpoint}°C`);
      if (data.rpm_setpoint !== prevState.rpm_setpoint)
        addEvent(elapsedSec, `RPM changed to ${data.rpm_setpoint}`);

      if (data.mode !== prevState.mode) {
        const now = Date.now();
        if (currentMode && modeStartTime) {
          const durationSec = Math.floor((now - modeStartTime) / 1000);
          const durationStr = formatRunningTime(durationSec);
          addEvent(elapsedSec, `Mode "${currentMode}" ended after ${durationStr}`);
        }
        addEvent(elapsedSec, `Mode changed to ${data.mode}`);
        currentMode = data.mode;
        modeStartTime = now;
      }
    }

    prevState = {
      temp_setpoint: data.temp_setpoint,
      rpm_setpoint: data.rpm_setpoint,
      mode: data.mode
    };
  }

  function addEvent(timeSec, description) {
    const y = getTemperatureAt(timeSec) + 2;
    tempChart.data.datasets[1].data.push({ x: timeSec, y, description });
  }

  function getTemperatureAt(timeSec) {
    const index = fullData.labels.findIndex(t => t === timeSec);
    if (index !== -1) return fullData.temps[index];

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

  function updateChartAggregated() {
    const lineData = fullData.labels.map((x, i) => ({ x, y: fullData.temps[i] }));
    tempChart.data.labels = fullData.labels;
    tempChart.data.datasets[0].data = lineData;

    const maxY = Math.max(...lineData.map(d => d.y), ...tempChart.data.datasets[1].data.map(d => d.y || 0), 50);
    tempChart.options.scales.y.max = Math.ceil((maxY + 10) / 5) * 5;

    tempChart.update();
  }

  function renderEvents(events) {
    const addedTimes = new Set(tempChart.data.datasets[1].data.map(e => e.x));
    events.forEach(event => {
      const eventTime = Math.floor((new Date(event.time).getTime() - chartStartTime) / 1000);
      if (!addedTimes.has(eventTime)) {
        const y = getTemperatureAt(eventTime) + 2;
        tempChart.data.datasets[1].data.push({ x: eventTime, y, description: event.description });
      }
    });
  }

  // ---------------- Message Handling ----------------

  function handleWebSocketMessage(event) {
    try {
      const msg = JSON.parse(event.data);

      switch (msg.type) {
        case 'history':
          console.log('Received history:', msg.data);
          if (Array.isArray(msg.data)) {
            chartStartTime = new Date(msg.data[0]?.time).getTime();
            fullData.labels = [];
            fullData.temps = [];
            tempChart.data.datasets[1].data = [];

            msg.data.forEach(point => {
              const t = new Date(point.time).getTime();
              const elapsedSec = Math.floor((t - chartStartTime) / 1000);
              fullData.labels.push(elapsedSec);
              fullData.temps.push(point.temperature);
            });

            updateChartAggregated();
          }
        
          break;
        case 'dataUpdate':
          updateInfoBoxes(msg.data);
          updateChart(msg.data);
          if (!window.formSyncedOnce) syncForm(msg.data);

          if (!modeStartTime && msg.data.mode) {
            currentMode = msg.data.mode;
            modeStartTime = Date.now();
          }
          break;
        case 'events':
          renderEvents(msg.data);
          updateChartAggregated();
          break;
        case 'notepadList':
          populateNotepadList(msg.experiments);
          break;
        case 'notepadData':
          document.getElementById('notepadName').value = msg.experiment;
          document.getElementById('notepadArea').value = msg.notes || '';
          break;
        case 'notepadSaved':
          alert(`Notepad "${msg.experiment}" saved successfully.`);
          refreshNotepadList();
          break;
        case 'error':
          alert('Error: ' + msg.message);
          break;
        default:
          console.warn('Unknown message type:', msg.type, 'Full message:', msg);
      }
    } catch (e) {
      console.error('WebSocket message error:', e);
    }
  }

  // function handleHistory(data) {
  //   if (!data.length) {
  //     chartStartTime = Date.now();
  //   } else {
  //     chartStartTime = new Date(data[0].time).getTime();
  //     fullData.labels = [];
  //     fullData.temps = [];
  //     tempChart.data.datasets[1].data = [];
  //     data.forEach(point => {
  //       const elapsedSec = Math.floor((new Date(point.time).getTime() - chartStartTime) / 1000);
  //       fullData.labels.push(elapsedSec);
  //       fullData.temps.push(point.temperature);
  //     });
  //     updateChartAggregated();
  //   }
  // }

  // ---------------- UI Sync ----------------

  function updateInfoBoxes(data) {
    $('#temp').text(`${data.temperature.toFixed(1)} °C`);
    $('#rpm').text(data.rpm);
    $('#mode').text(data.mode);
    $('#runningTime').text(formatRunningTime(data.running_time));
  }

  function formatRunningTime(rt) {
    if (typeof rt !== 'number' || isNaN(rt) || rt < 0) return '--:--';
    const m = Math.floor(rt / 60);
    const s = Math.floor(rt % 60);
    return `${m}:${s.toString().padStart(2, '0')}`;
  }

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

  function sendControlUpdate(updateData) {
    sendMessage({ action: 'controlUpdate', data: updateData });
  }
  function showDashboardNotification(message, type = 'success') {
    // Remove any existing notification
    let existing = document.getElementById('dashboardNotification');
    if (existing) existing.remove();

    // Create notification div
    const notif = document.createElement('div');
    notif.id = 'dashboardNotification';
    notif.textContent = message;
    notif.className = `alert alert-${type}`;
    notif.style.position = 'fixed';
    notif.style.top = '20px';
    notif.style.right = '20px';
    notif.style.zIndex = 9999;
    notif.style.minWidth = '200px';
    notif.style.textAlign = 'center';
    notif.style.boxShadow = '0 2px 8px rgba(0,0,0,0.2)';
    document.body.appendChild(notif);

    setTimeout(() => {
      notif.remove();
    }, 2500);
  }
  function showStatusMessage(msg, type = 'info') {
    let statusDiv = document.getElementById('notepadStatus');
    if (!statusDiv) {
      statusDiv = document.createElement('div');
      statusDiv.id = 'notepadStatus';
      document.body.appendChild(statusDiv);
    }
    statusDiv.textContent = msg;
    statusDiv.className = `mt-2 alert alert-${type}`;
    statusDiv.style.display = 'block';
    setTimeout(() => {
      statusDiv.style.display = 'none';
    }, 2500);
  }

  function setupControlHandlers() {
    $('#modeSelect').on('change', updateModeFields);

    $('#controlForm').on('submit', (e) => {
      e.preventDefault();
      const data = {
        temp_setpoint: parseFloat($('#tempSetpoint').val()) || undefined,
        rpm_setpoint: parseInt($('#rpmSetpoint').val()) || undefined,
        mode: $('#modeSelect').val()
      };
      if (data.mode === 'Ramp') data.ramp_rate = parseFloat($('#rampRate').val()) || undefined;
      if (data.mode === 'Timer') data.duration = parseInt($('#timerDuration').val()) || undefined;
      sendControlUpdate(data);
      showDashboardNotification('Settings updated successfully.', 'success');
      window.formSyncedOnce = false;
    });

    $('#alertRpmThreshold').on('change', e => {
      sendControlUpdate({ alertRpmThreshold: parseFloat(e.target.value) });
    });

    $('#alertTimerThreshold').on('change', e => {
      sendControlUpdate({ alertTimerThreshold: parseFloat(e.target.value) });
    });
  }

  // ---------------- Init Everything ----------------

  function initializeApp() {
    initWebSocket();
    initChart();
    setupNotepadHandlers();
    setupControlHandlers();
  }

  initializeApp();
});
