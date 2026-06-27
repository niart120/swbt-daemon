#include "btstack_bridge/production_ports.h"

#include "switch/switch_hid_descriptor.h"

static bool swbt_btstack_production_ipc_pump_port_is_valid(
    const swbt_btstack_production_ipc_pump_port_t *port) {
    return port != NULL && port->start != NULL && port->stop != NULL;
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
