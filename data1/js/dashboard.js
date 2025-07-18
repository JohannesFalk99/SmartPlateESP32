document.addEventListener('DOMContentLoaded', () => {
  function getTemperatureAt(timeSec) {
  const index = fullData.labels.findIndex(t => t === timeSec);
  if (index !== -1) {
    return fullData.temps[index];
  }

  // Fallback: get closest
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
      animation: {
        duration: 800,
        easing: 'easeOutQuart'
      },
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
              if (ctx.dataset.label === 'Events') {
                return ctx.raw.description;
              } else {
                return `Temp: ${ctx.raw.y.toFixed(2)} °C`;
              }
            }
          }
        }
      }
    }
  });

  let chartStartTime = null;
  let autoScroll = true;
  let fullData = { labels: [], temps: [] };
  let prevState = {};

  document.getElementById('tempChart').addEventListener('mouseenter', () => autoScroll = false);
  document.getElementById('tempChart').addEventListener('mouseleave', () => autoScroll = true);

  function formatTime(seconds) {
    if (seconds < 60) return `${seconds}s`;
    return `${Math.floor(seconds / 60)}m`;
  }

  function addEvent(timeSec, description) {
  const tempAtTime = getTemperatureAt(timeSec);
  const y = tempAtTime + 2; // place it just above the temp line

  tempData.datasets[1].data.push({
    x: timeSec,
    y,
    description
  });
}
function renderEvents(events) {
  const addedTimes = new Set(tempData.datasets[1].data.map(e => e.x));
  events.forEach(event => {
    const eventTime = Math.floor((new Date(event.time).getTime() - chartStartTime) / 1000);
    if (!addedTimes.has(eventTime)) {
      const y = getTemperatureAt(eventTime) + 2;
      tempData.datasets[1].data.push({
        x: eventTime,
        y,
        description: event.description
      });
    }
  });
}


  function updateChartAggregated() {
    const maxElapsed = fullData.labels.at(-1) || 0;

    // if (maxElapsed < 60) {
      tempData.labels = fullData.labels;
      tempData.datasets[0].data = fullData.labels.map((x, i) => ({ x, y: fullData.temps[i] }));
    // } else {
    //   const grouped = {};
    //   fullData.labels.forEach((sec, i) => {
    //     const min = Math.floor(sec / 60);
    //     if (!grouped[min]) grouped[min] = [];
    //     grouped[min].push(fullData.temps[i]);
    //   });

    //   const mins = Object.keys(grouped).map(Number).sort((a, b) => a - b);
    //   tempData.datasets[0].data = mins.map(min => ({
    //     x: min * 60,
    //     y: grouped[min].reduce((a, b) => a + b, 0) / grouped[min].length
    //   }));
    //   tempData.labels = mins.map(m => m * 60);
    // }

    // Good: compute maxY
const maxY = Math.max(
  ...tempData.datasets[0].data.map(d => d.y),
  ...tempData.datasets[1].data.map(d => d.y || 0),
  50
);

// Good: adjust Y axis
tempChart.options.scales.y.max = Math.ceil((maxY + 5) / 5) * 5;

    // tempData.datasets[1].data.forEach(event => event.y = maxY + 5);

    tempChart.options.scales.y.max = Math.ceil((maxY + 10) / 5) * 5;
    // if (autoScroll) 
      tempChart.update();
  }

  function updateChart(data) {
    if (!chartStartTime) chartStartTime = Date.now();

    const elapsedSec = Math.floor((Date.now() - chartStartTime) / 1000);
    fullData.labels.push(elapsedSec);
    fullData.temps.push(data.temperature);

    if (prevState.temp_setpoint !== undefined) {
      if (data.temp_setpoint !== prevState.temp_setpoint) {
        addEvent(elapsedSec, `Temp setpoint changed to ${data.temp_setpoint}°C`);
      }
      if (data.rpm_setpoint !== prevState.rpm_setpoint) {
        addEvent(elapsedSec, `RPM changed to ${data.rpm_setpoint}`);
      }
      if (data.mode !== prevState.mode) {
        addEvent(elapsedSec, `Mode changed to ${data.mode}`);
      }
    }

    prevState = {
      temp_setpoint: data.temp_setpoint,
      rpm_setpoint: data.rpm_setpoint,
      mode: data.mode
    };

    updateChartAggregated();
  }

  function updateInfoBoxes(data) {
    $('#temp').text(`${data.temperature.toFixed(1)} °C`);
    $('#rpm').text(data.rpm);
    $('#mode').text(data.mode);
    const rt = data.running_time;
    $('#runningTime').text(rt < 60 ? `${rt}s` : `${Math.floor(rt / 60)}m ${Math.floor(rt % 60)}s`);
  }

function fetchData() {
  $.getJSON('/api/data')
    .done(data => {
      updateInfoBoxes(data);
      updateChart(data);
      if (!window.formSyncedOnce) syncForm(data);
    });

  // Fetch and add any new events
  $.getJSON('/api/events')
    .done(events => {
      renderEvents(events);
    });
}


  function syncForm(data) {
    $('#tempSetpoint').val(data.temp_setpoint ?? '');
    $('#rpmSetpoint').val(data.rpm_setpoint ?? '');
    $('#modeSelect').val(data.mode ?? 'Hold');
    $('#rampRate').val('');
    $('#timerDuration').val('');
    if (data.mode === 'Ramp') $('#rampRate').val(data.ramp_rate ?? '');
    if (data.mode === 'Timer') $('#timerDuration').val(data.duration ?? '');
    updateModeFields();
    window.formSyncedOnce = true;
  }

  function updateModeFields() {
    const mode = $('#modeSelect').val();
    $('.mode-field').hide();
    if (mode === 'Ramp') $('#rampRateGroup').show();
    else if (mode === 'Timer') $('#timerDurationGroup').show();
  }

  function submitControlForm(e) {
    e.preventDefault();
    const payload = {
      temp_setpoint: parseFloat($('#tempSetpoint').val()) || undefined,
      rpm_setpoint: parseInt($('#rpmSetpoint').val()) || undefined,
      mode: $('#modeSelect').val()
    };
    if (payload.mode === 'Ramp') payload.ramp_rate = parseFloat($('#rampRate').val()) || undefined;
    if (payload.mode === 'Timer') payload.duration = parseInt($('#timerDuration').val()) || undefined;

    $.ajax({
      url: '/api/control',
      type: 'POST',
      contentType: 'application/json',
      data: JSON.stringify(payload),
      success: () => {
        alert('Settings updated');
        window.formSyncedOnce = false;
      },
      error: () => alert('Failed to update settings')
    });
  }

  $('#modeSelect').on('change', updateModeFields);
  $('#controlForm').on('submit', submitControlForm);

  // Initialize with history
  $.getJSON('/api/history')
    .done(history => {
      if (!history.length) chartStartTime = Date.now();
      else {
        chartStartTime = new Date(history[0].time).getTime();
        history.forEach(point => {
          const elapsedSec = Math.floor((new Date(point.time).getTime() - chartStartTime) / 1000);
          fullData.labels.push(elapsedSec);
          fullData.temps.push(point.temperature);
        });
        updateChartAggregated();
      }
    });

  fetchData();
  setInterval(fetchData, 1000);
});
