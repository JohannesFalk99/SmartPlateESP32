// // controls.js

// function updateData() {
//   $.getJSON('/api/data', function(data) {
//     $('#temp').text(data.temperature + ' °C');
//     $('#rpm').text(data.rpm);
//     $('#mode').text(data.mode);

//     // Update input controls to reflect current setpoints
//     $('#tempSetpoint').val(data.temp_setpoint);
//     $('#modeSelect').val(data.mode);
//     $('#rpmSetpoint').val(data.rpm_setpoint);

//     // Add new data point to chart
//     const now = new Date().toLocaleTimeString();
//     tempData.labels.push(now);
//     tempData.datasets[0].data.push(data.temperature);

//     // Keep last 20 data points
//     if(tempData.labels.length > 20) {
//       tempData.labels.shift();
//       tempData.datasets[0].data.shift();
//     }
//     tempChart.update();
//   }).fail(function() {
//     console.error("Failed to fetch /api/data");
//   });
// }

// function sendControls() {
//   const tempSetpoint = parseFloat($('#tempSetpoint').val());
//   const mode = $('#modeSelect').val();
//   const rpmSetpoint = parseInt($('#rpmSetpoint').val());

//   $.ajax({
//     url: '/api/control',
//     method: 'POST',
//     contentType: 'application/json',
//     data: JSON.stringify({
//       temp_setpoint: tempSetpoint,
//       mode: mode,
//       rpm_setpoint: rpmSetpoint
//     }),
//     success: function(response) {
//       console.log("Settings updated");
//     },
//     error: function() {
//       alert("Failed to update settings");
//     }
//   });
// }

// $(document).ready(function() {
//   updateData();
//   setInterval(updateData, 2000);

//   $('#updateBtn').on('click', function() {
//     sendControls();
//   });
// });
