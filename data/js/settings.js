// $(function () {
//     // Mode selection change handler — only controls mode-related fields visibility
//     $('#modeSelect').on('change', function () {
//         const mode = $(this).val();
//         $('.mode-field').hide();
//         if (mode === 'Ramp') {
//             $('#rampRateGroup').show();
//         } else if (mode === 'Hold') {
//             $('#holdDurationGroup').show();
//         } else if (mode === 'Timer') {
//             $('#timerDurationGroup').show();
//         }
//     });

//     // Initialize mode fields visibility on page load
//     $('#modeSelect').trigger('change');

//     // Settings form submission handler
//     $('#settingsForm').on('submit', function (e) {
//         e.preventDefault();

//         const settingsData = {
//             alertTempThreshold: parseFloat($('#alertTempThreshold').val()) || null,
//             alertRpmThreshold: parseFloat($('#alertRpmThreshold').val()) || null,
//             alertTimerThreshold: parseFloat($('#alertTimerThreshold').val()) || null
//         };

//         $.ajax({
//             url: '/api/control',
//             method: 'POST',
//             contentType: 'application/json',
//             data: JSON.stringify(settingsData),
//             success: function () {
//                 alert('Settings saved successfully!');
//             },
//             error: function () {
//                 alert('Failed to save settings.');
//             }
//         });
//     });
// });
