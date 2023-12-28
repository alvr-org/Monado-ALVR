# Copyright 2020-2023, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
# Author: Jakob Bornecrantz <jakob@collabora.com>
# Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
# Author: Korcan Hussein <korcan.hussein@collabora.com>

import os
from cffi import FFI
from pathlib import Path


def preprocess_ffi(lines: list[str]) -> str:
    out_lines = []
    skipping = False
    for line in lines:
        if line.startswith("#ifdef"):
            skipping = True
            continue
        elif line.startswith("#endif"):
            skipping = False
            continue
        elif line.startswith("#include"):
            continue
        if skipping:
            continue
        out_lines.append(line)

    return "\n".join(out_lines)


class MonadoHeaderNotFoundError(Exception):
    pass


def find_monado_header() -> Path:
    search_paths = [
        # Try script directory first, in case running uninstalled
        Path(__file__).parent / "monado.h",
        # path from installed package
        Path("/usr/include/monado/monado.h"),
        # path from local install
        Path("/usr/local/include/monado/monado.h")
    ]

    # path from ENV
    if os.getenv('MONADO_HEADER_PATH'):
        search_paths.append(Path(os.getenv('MONADO_HEADER_PATH')))

    for path in search_paths:
        if path.is_file():
            return path

    # Give up
    raise MonadoHeaderNotFoundError("Could not find monado.h. Define the path by setting MONADO_HEADER_PATH.")


def load_ffi() -> FFI:
    ffi = FFI()

    header_path = find_monado_header()
    with header_path.open("r") as f:
        header_lines = f.readlines()

    header = preprocess_ffi(header_lines)

    ffi.cdef(header)
    return ffi


class Device:
    def __init__(self, name, serial):
        self.name = name
        self.serial = serial


class Client:
    def __init__(self, ident, name, primary, focused, visible, active, overlay, io_active):
        self.ident = ident
        self.name = name
        self.primary = primary
        self.focused = focused
        self.visible = visible
        self.active = active
        self.overlay = overlay
        self.io_active = io_active


class MonadoLibraryNotFoundError(Exception):
    pass


class Monado:
    def __init__(self):
        self.ffi = load_ffi()
        try:
            self.lib = self.ffi.dlopen("libmonado.so")
        except OSError:
            raise MonadoLibraryNotFoundError("Could not load libmonado.so.")

        self.root_ptr = self.ffi.new("mnd_root_t **")

        ret = self.lib.mnd_root_create(self.root_ptr)
        if ret != 0:
            raise Exception("Could not create root")

        self.root = self.root_ptr[0]
        self.client_count = 0
        self.client_count_ptr = self.ffi.new("uint32_t *")
        self.name_ptr = self.ffi.new("char **")
        self.flags_ptr = self.ffi.new("uint32_t *")
        self.client_id_ptr = self.ffi.new("uint32_t *")
        self.device_name_ptr = self.ffi.new("char **")
        self.device_serial_ptr = self.ffi.new("char **")
        self.device_count_ptr = self.ffi.new("uint32_t *")

    def update_clients(self):
        ret = self.lib.mnd_root_update_client_list(self.root)
        if ret != 0:
            raise Exception("Could not update clients")

        ret = self.lib.mnd_root_get_number_clients(self.root, self.client_count_ptr)
        if ret != 0:
            raise Exception("Could not update clients")

        self.client_count = self.client_count_ptr[0]

    def get_client_id_at_index(self, index):
        ret = self.lib.mnd_root_get_client_id_at_index(self.root, index, self.client_id_ptr)
        if ret != 0:
            raise Exception("Could not get client id at index")

        return self.client_id_ptr[0]

    def get_client_name(self, client_id):
        ret = self.lib.mnd_root_get_client_name(self.root, client_id, self.name_ptr)
        if ret != 0:
            raise Exception("Could not get client name")

        return self.ffi.string(self.name_ptr[0]).decode("utf-8")

    def get_client_flags(self, client_id):
        ret = self.lib.mnd_root_get_client_state(self.root, client_id, self.flags_ptr)
        if ret != 0:
            raise Exception("Could not get client state")

        return self.flags_ptr[0]

    def snapshot_client(self, index):
        ident = self.get_client_id_at_index(index)
        name = self.get_client_name(ident)
        flags = self.get_client_flags(ident)

        primary = (flags & self.lib.MND_CLIENT_PRIMARY_APP) != 0
        active = (flags & self.lib.MND_CLIENT_SESSION_ACTIVE) != 0
        visible = (flags & self.lib.MND_CLIENT_SESSION_VISIBLE) != 0
        focused = (flags & self.lib.MND_CLIENT_SESSION_FOCUSED) != 0
        overlay = (flags & self.lib.MND_CLIENT_SESSION_OVERLAY) != 0
        io_active = (flags & self.lib.MND_CLIENT_IO_ACTIVE) != 0

        return Client(ident, name, primary, focused, visible, active, overlay, io_active)

    def destroy(self):
        self.lib.mnd_root_destroy(self.root_ptr)
        self.root = self.root_ptr[0]

    def set_primary(self, client_id: int):
        ret = self.lib.mnd_root_set_client_primary(self.root, client_id)
        if ret != 0:
            raise Exception(f"Failed to set primary client id to {client_id}.")

    def set_focused(self, client_id: int):
        ret = self.lib.mnd_root_set_client_focused(self.root, client_id)
        if ret != 0:
            raise Exception(f"Failed to set focused client id to {client_id}.")

    def toggle_io(self, client_id: int):
        ret = self.lib.mnd_root_toggle_client_io_active(self.root, client_id)
        if ret != 0:
            raise Exception(f"Failed to toggle io for client id {client_id}.")

    def get_device_count(self):
        ret = self.lib.mnd_root_get_device_count(self.root, self.device_count_ptr)
        if ret != 0:
            raise Exception("Could not get device count")
        return self.device_count_ptr[0]

    def get_device_at_index(self, index):
        prop = self.lib.MND_PROPERTY_NAME_STRING
        ret = self.lib.mnd_root_get_device_info_string(self.root, index, prop, self.device_name_ptr)
        if ret != 0:
            raise Exception(f"Could not get device name at index:{index}")

        prop = self.lib.MND_PROPERTY_SERIAL_STRING
        ret = self.lib.mnd_root_get_device_info_string(self.root, index, prop, self.device_serial_ptr)
        if ret != 0:
            raise Exception(f"Could not get device serial at index:{index}")

        dev_name = self.ffi.string(self.device_name_ptr[0]).decode("utf-8")
        dev_serial = self.ffi.string(self.device_serial_ptr[0]).decode("utf-8")

        return Device(dev_name, dev_serial)

    def get_devices(self):
        devices = []
        dev_count = self.get_device_count()
        for i in range(dev_count):
            devices.append(self.get_device_at_index(i))
        return devices

    def get_device_roles(self):
        role_map = dict()
        device_int_id_ptr = self.ffi.new("int32_t *")
        for role_name in ["head", "left", "right", "gamepad", "eyes", "hand-tracking-left", "hand-tracking-right"]:
            crole_name = role_name.encode('utf-8')
            ret = self.lib.mnd_root_get_device_from_role(self.root, crole_name, device_int_id_ptr)
            if ret != 0:
                raise Exception(f"Could not get device role: {role_name}")
            role_map[role_name] = device_int_id_ptr[0]
        return role_map
