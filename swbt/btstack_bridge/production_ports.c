#include "btstack_bridge/production_ports.h"

#include "switch/switch_hid_descriptor.h"

static bool swbt_btstack_production_ipc_pump_port_is_valid(
    const swbt_btstack_production_ipc_pump_port_t *port) {
    return port != NULL && port->start != NULL && port->stop != NULL;
}

static bool swbt_btstack_production_device_port_is_valid(const swbt_btstack_device_port_t *port) {
    return port != NULL && port->platform_start != NULL && port->platform_stop != NULL &&
           port->hid_register != NULL && port->hid_stop != NULL && port->connect != NULL &&
           port->send != NULL;
}

static bool swbt_btstack_production_output_handler_port_is_valid(
    const swbt_btstack_production_output_handler_port_t *port) {
    return port != NULL && port->start != NULL && port->stop != NULL;
}

static bool swbt_btstack_production_report_timer_port_is_valid(
    const swbt_btstack_production_report_timer_port_t *port) {
    return port != NULL && port->init != NULL && port->start != NULL &&
           port->on_can_send_now != NULL && port->enqueue_subcommand_reply != NULL &&
           port->stop != NULL && port->send_neutral_now != NULL;
}

static bool swbt_btstack_production_controller_port_is_valid(
    const swbt_btstack_production_controller_port_t *port) {
    return port != NULL && port->confirm_ssp_user_confirmation != NULL &&
           port->read_controller_address != NULL;
}

static bool
swbt_btstack_production_clock_port_is_valid(const swbt_btstack_production_clock_port_t *port) {
    return port != NULL && port->time_ms != NULL;
}

static bool
swbt_btstack_production_power_port_is_valid(const swbt_btstack_production_power_port_t *port) {
    return port != NULL && port->on != NULL && port->off != NULL;
}

static bool swbt_btstack_production_run_loop_port_is_valid(
    const swbt_btstack_production_run_loop_port_t *port) {
    return port != NULL && port->execute != NULL && port->execute_on_main_thread != NULL &&
           port->trigger_exit != NULL;
}

swbt_btstack_hid_registration_config_t swbt_btstack_production_hid_registration_config(void) {
    return (swbt_btstack_hid_registration_config_t){
        .hid_device_subclass = 0x2508u,
        .hid_country_code = 0x21u,
        .hid_virtual_cable = 1u,
        .hid_remote_wake = 1u,
        .hid_reconnect_initiate = 1u,
        .hid_normally_connectable = true,
        .hid_boot_device = false,
        .hid_ssr_host_max_latency = 0xffffu,
        .hid_ssr_host_min_timeout = 0xffffu,
        .hid_supervision_timeout = 0x0c80u,
        .hid_descriptor = swbt_switch_hid_descriptor_data(),
        .hid_descriptor_size = (uint16_t)swbt_switch_hid_descriptor_size(),
        .device_name = "Pro Controller",
        .packet_handler = NULL,
    };
}

bool swbt_btstack_production_ports_has_ipc_pump(const swbt_btstack_production_ports_t *ports) {
    return ports != NULL && swbt_btstack_production_ipc_pump_port_is_valid(&ports->ipc_pump);
}

bool swbt_btstack_production_ports_is_valid(const swbt_btstack_production_ports_t *ports) {
    return swbt_btstack_production_ports_has_ipc_pump(ports) &&
           swbt_btstack_production_device_port_is_valid(&ports->device) &&
           swbt_btstack_production_output_handler_port_is_valid(&ports->output_handler) &&
           swbt_btstack_production_report_timer_port_is_valid(&ports->report_timer) &&
           swbt_btstack_production_controller_port_is_valid(&ports->controller) &&
           swbt_btstack_production_clock_port_is_valid(&ports->clock) &&
           swbt_btstack_production_power_port_is_valid(&ports->power) &&
           swbt_btstack_production_run_loop_port_is_valid(&ports->run_loop);
}
