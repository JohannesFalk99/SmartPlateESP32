$(function () {
    // Preload existing alert thresholds from backend on page load
    function loadAlertThresholds() {
        $.get('/api/alerts')
            .done((data) => {
                if (data.alertTempThreshold !== undefined) $('#tempAlert').val(data.alertTempThreshold);
                if (data.alertRpmThreshold !== undefined) $('#rpmAlert').val(data.alertRpmThreshold);
                if (data.alertTimerThreshold !== undefined) $('#timerAlert').val(data.alertTimerThreshold);
            })
            .fail(() => {
                console.warn('Failed to load alert thresholds from backend.');
            });
    }

    loadAlertThresholds();

    // Handle form submission
    $('#alertsForm').on('submit', function (e) {
        e.preventDefault();

        const alertThresholds = {
            alertTempThreshold: parseFloat($('#tempAlert').val()) || null,
            alertRpmThreshold: parseFloat($('#rpmAlert').val()) || null,
            alertTimerThreshold: parseFloat($('#timerAlert').val()) || null
        };

        // Disable submit button while saving
        const $submitBtn = $(this).find('button[type="submit"]');
        $submitBtn.prop('disabled', true).text('Saving...');

        $.ajax({
            url: '/api/alerts',
            method: 'POST',
            contentType: 'application/json',
            data: JSON.stringify(alertThresholds),
            success: () => {
                alert('Alert thresholds saved!');
            },
            error: () => {
                alert('Failed to save alert thresholds.');
            },
            complete: () => {
                $submitBtn.prop('disabled', false).text('Save Alert Settings');
            }
        });
    });
});
