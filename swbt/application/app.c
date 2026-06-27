#include "application/app.h"

#include <stddef.h>
#include <stdlib.h>

#include "core/spin_lock.h"
#include "switch/switch_player_lights.h"
#include "switch/switch_spi.h"
#include "switch/switch_spi_seed.h"

struct swbt_app {
    swbt_spin_lock_t lock;
    swbt_control_lease_t lease;
    swbt_state_t state;
    swbt_metrics_t metrics;
    swbt_app_daemon_status_t daemon;
    swbt_app_hardware_status_t hardware;
    swbt_switch_rumble_state_t rumble;
    swbt_switch_spi_t spi;
    swbt_switch_player_lights_state_t player_lights;
};

static swbt_app_result_t swbt_app_map_lease_result(swbt_control_lease_result_t result) {
    switch (result) {
    case SWBT_CONTROL_LEASE_OK:
        return SWBT_APP_OK;
    case SWBT_CONTROL_LEASE_ERROR_OWNER_BUSY:
        return SWBT_APP_ERROR_OWNER_BUSY;
    case SWBT_CONTROL_LEASE_ERROR_NOT_OWNER:
        return SWBT_APP_ERROR_NOT_OWNER;
    }
    return SWBT_APP_ERROR_INVALID_ARGUMENT;
}

static swbt_app_daemon_status_t swbt_app_daemon_status_default(void) {
    return (swbt_app_daemon_status_t){
        .backend = SWBT_APP_DAEMON_BACKEND_UNKNOWN,
        .lifecycle_state = SWBT_APP_DAEMON_LIFECYCLE_STOPPED,
        .hardware_approval = SWBT_APP_HARDWARE_APPROVAL_UNAVAILABLE,
    };
}

static swbt_app_hardware_status_t swbt_app_hardware_status_default(void) {
    return (swbt_app_hardware_status_t){
        .adapter_state = SWBT_APP_HARDWARE_CHANNEL_UNAVAILABLE,
        .switch_connection_state = SWBT_APP_HARDWARE_CHANNEL_UNAVAILABLE,
        .hid_channel_state = SWBT_APP_HARDWARE_CHANNEL_UNAVAILABLE,
    };
}

static void swbt_app_apply_neutral(swbt_app_t *app) {
    app->state = swbt_state_neutral();
}

static void swbt_app_revoke_all(swbt_app_t *app) {
    swbt_control_lease_revoke(&app->lease);
    swbt_app_apply_neutral(app);
}

static void swbt_app_record_state_update_rejected_locked(swbt_app_t *app) {
    (void)swbt_metrics_record_state_update_rejected(&app->metrics);
}

static swbt_app_result_t swbt_app_init(swbt_app_t *app) {
    const swbt_switch_spi_seed_profile_t spi_profile = swbt_switch_spi_seed_dev_profile();

    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    *app = (swbt_app_t){0};
    swbt_spin_lock_init(&app->lock);
    swbt_control_lease_init(&app->lease);
    app->daemon = swbt_app_daemon_status_default();
    app->hardware = swbt_app_hardware_status_default();
    swbt_app_apply_neutral(app);
    if (swbt_metrics_init(&app->metrics) != SWBT_METRICS_OK ||
        swbt_switch_rumble_init(&app->rumble) != SWBT_SWITCH_RUMBLE_OK ||
        swbt_switch_spi_init(&app->spi) != SWBT_SWITCH_SPI_OK ||
        swbt_switch_spi_seed_apply(&app->spi, &spi_profile) != SWBT_SWITCH_SPI_OK ||
        swbt_switch_player_lights_init(&app->player_lights) != SWBT_SWITCH_PLAYER_LIGHTS_OK) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }
    return SWBT_APP_OK;
}

swbt_app_t *swbt_app_create(void) {
    swbt_app_t *app = malloc(sizeof(*app));

    if (app == NULL) {
        return NULL;
    }
    if (swbt_app_init(app) != SWBT_APP_OK) {
        free(app);
        return NULL;
    }
    return app;
}

void swbt_app_destroy(swbt_app_t *app) {
    free(app);
}

swbt_app_result_t swbt_app_acquire(swbt_app_t *app, uint32_t client_id) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    const swbt_app_result_t result =
        swbt_app_map_lease_result(swbt_control_lease_acquire(&app->lease, client_id));
    swbt_spin_lock_release(&app->lock);
    return result;
}

swbt_app_result_t swbt_app_set_state(swbt_app_t *app, swbt_app_set_state_options_t options) {
    const uint32_t client_id = options.client_id;
    const swbt_state_t *state = options.state;
    const uint64_t sequence = options.sequence;

    if (app == NULL || state == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    const swbt_control_lease_snapshot_t lease = swbt_control_lease_snapshot(&app->lease);
    if (!lease.has_owner || lease.owner_client_id != client_id) {
        swbt_app_record_state_update_rejected_locked(app);
        swbt_spin_lock_release(&app->lock);
        return SWBT_APP_ERROR_NOT_OWNER;
    }
    if (sequence < lease.last_sequence) {
        swbt_app_record_state_update_rejected_locked(app);
        swbt_spin_lock_release(&app->lock);
        return SWBT_APP_ERROR_STALE_SEQUENCE;
    }

    const swbt_app_result_t sequence_result =
        swbt_app_map_lease_result(swbt_control_lease_accept_sequence(
            &app->lease, (swbt_control_lease_accept_sequence_options_t){
                             .client_id = client_id,
                             .sequence = sequence,
                         }));
    if (sequence_result != SWBT_APP_OK) {
        swbt_app_record_state_update_rejected_locked(app);
        swbt_spin_lock_release(&app->lock);
        return sequence_result;
    }

    app->state = *state;
    (void)swbt_metrics_record_state_update_accepted(&app->metrics, 0u);
    swbt_spin_lock_release(&app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_revoke(swbt_app_t *app, swbt_app_revoke_options_t options) {
    const swbt_app_revoke_reason_t reason = options.reason;
    const uint32_t client_id = options.client_id;

    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    switch (reason) {
    case SWBT_APP_REVOKE_RELEASE:
        if (swbt_control_lease_release(&app->lease, client_id) != SWBT_CONTROL_LEASE_OK) {
            swbt_spin_lock_release(&app->lock);
            return SWBT_APP_ERROR_NOT_OWNER;
        }
        swbt_app_apply_neutral(app);
        swbt_spin_lock_release(&app->lock);
        return SWBT_APP_OK;
    case SWBT_APP_REVOKE_DISCONNECT:
    case SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT:
        if (swbt_control_lease_revoke_if_owner(&app->lease, client_id)) {
            swbt_app_apply_neutral(app);
        }
        swbt_spin_lock_release(&app->lock);
        return SWBT_APP_OK;
    case SWBT_APP_REVOKE_SHUTDOWN:
        swbt_app_revoke_all(app);
        swbt_spin_lock_release(&app->lock);
        return SWBT_APP_OK;
    }

    swbt_spin_lock_release(&app->lock);
    return SWBT_APP_ERROR_INVALID_ARGUMENT;
}

swbt_app_result_t swbt_app_read_controller_state(const swbt_app_t *app, swbt_state_t *out_state) {
    if (app == NULL || out_state == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_app_t *mutable_app = (swbt_app_t *)app;
    swbt_spin_lock_acquire(&mutable_app->lock);
    *out_state = app->state;
    swbt_spin_lock_release(&mutable_app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_read_status(const swbt_app_t *app,
                                       swbt_app_status_snapshot_t *out_status) {
    if (app == NULL || out_status == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_app_t *mutable_app = (swbt_app_t *)app;
    swbt_spin_lock_acquire(&mutable_app->lock);
    const swbt_control_lease_snapshot_t lease = swbt_control_lease_snapshot(&app->lease);
    out_status->has_owner = lease.has_owner;
    out_status->owner_client_id = lease.owner_client_id;
    out_status->last_sequence = lease.last_sequence;
    out_status->state = app->state;
    out_status->rumble = app->rumble;
    if (swbt_metrics_snapshot(&app->metrics, &out_status->metrics) != SWBT_METRICS_OK) {
        swbt_spin_lock_release(&mutable_app->lock);
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }
    out_status->daemon = app->daemon;
    out_status->hardware = app->hardware;
    swbt_spin_lock_release(&mutable_app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_set_daemon_status(swbt_app_t *app,
                                             const swbt_app_daemon_status_t *daemon_status) {
    if (app == NULL || daemon_status == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    app->daemon = *daemon_status;
    swbt_spin_lock_release(&app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_set_daemon_lifecycle(swbt_app_t *app,
                                                swbt_app_daemon_lifecycle_state_t lifecycle_state) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    app->daemon.lifecycle_state = lifecycle_state;
    swbt_spin_lock_release(&app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_set_hardware_approval(swbt_app_t *app,
                                                 swbt_app_hardware_approval_t hardware_approval) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    app->daemon.hardware_approval = hardware_approval;
    swbt_spin_lock_release(&app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_set_hardware_status(swbt_app_t *app,
                                               const swbt_app_hardware_status_t *hardware_status) {
    if (app == NULL || hardware_status == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    app->hardware = *hardware_status;
    swbt_spin_lock_release(&app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_record_report_tick(swbt_app_t *app, uint64_t now_us,
                                              swbt_metrics_report_send_result_t send_result) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    const swbt_metrics_result_t result =
        swbt_metrics_record_report_tick(&app->metrics, (swbt_metrics_report_tick_options_t){
                                                           .now_us = now_us,
                                                           .send_result = send_result,
                                                       });
    swbt_spin_lock_release(&app->lock);
    return result == SWBT_METRICS_OK ? SWBT_APP_OK : SWBT_APP_ERROR_INVALID_ARGUMENT;
}

swbt_app_result_t swbt_app_record_state_update_rejected(swbt_app_t *app) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    swbt_app_record_state_update_rejected_locked(app);
    swbt_spin_lock_release(&app->lock);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_record_rumble(swbt_app_t *app, const uint8_t *payload,
                                         uint64_t updated_at_ms) {
    if (app == NULL || payload == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    const swbt_switch_rumble_result_t result =
        swbt_switch_rumble_update(&app->rumble, payload, updated_at_ms);
    swbt_spin_lock_release(&app->lock);
    return result == SWBT_SWITCH_RUMBLE_OK ? SWBT_APP_OK : SWBT_APP_ERROR_INVALID_ARGUMENT;
}

swbt_app_result_t
swbt_app_handle_output_report(swbt_app_t *app, const swbt_switch_output_report_t *output_report,
                              const swbt_switch_report_options_t *report_options,
                              const swbt_switch_device_info_t *device_info, uint64_t updated_at_ms,
                              swbt_switch_subcommand_dispatcher_response_t *out_response) {
    if (app == NULL || output_report == NULL || report_options == NULL || device_info == NULL ||
        out_response == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&app->lock);
    if (swbt_switch_rumble_update(&app->rumble, output_report->rumble, updated_at_ms) !=
        SWBT_SWITCH_RUMBLE_OK) {
        swbt_spin_lock_release(&app->lock);
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    const swbt_switch_subcommand_dispatcher_config_t dispatch_config = {
        .state = &app->state,
        .report_options = report_options,
        .spi = &app->spi,
        .player_lights = &app->player_lights,
        .device_info = device_info,
    };
    const swbt_switch_subcommand_dispatch_result_t dispatch_result =
        swbt_switch_subcommand_dispatch(&dispatch_config, output_report, out_response);
    swbt_spin_lock_release(&app->lock);

    return dispatch_result == SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK ? SWBT_APP_OK
                                                                 : SWBT_APP_ERROR_INVALID_ARGUMENT;
}
