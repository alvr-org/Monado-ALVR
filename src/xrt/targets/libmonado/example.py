#!/bin/env python3
# Copyright 2020-2023, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
# Author: Jakob Bornecrantz <jakob@collabora.com>
# Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
# Author: Korcan Hussein <korcan.hussein@collabora.com>

import argparse
from monado import Monado, MonadoLibraryNotFoundError, MonadoHeaderNotFoundError


def main():
    parser = argparse.ArgumentParser(description='libmonado Python example.')
    parser.add_argument("-f", "--focused", type=int, metavar='CLIENT_ID',
                        help="Set focused client")
    parser.add_argument("-p", "--primary", type=int, metavar='CLIENT_ID',
                        help="Set primary client")
    parser.add_argument("-i", "--input", type=int, metavar='CLIENT_ID',
                        help="Toggle whether client receives input")
    args = parser.parse_args()

    try:
        m = Monado()
    except MonadoLibraryNotFoundError:
        print("Could not find an installed libmonado.so in your path.")
        print("Add the Monado build directory to your LD_LIBRARY_PATH:")
        print(f"LD_LIBRARY_PATH=/home/user/monado/build/src/xrt/targets/libmonado/ ./{parser.prog}")
        return
    except MonadoHeaderNotFoundError:
        print("Could not find an installed monado.h.")
        print("Try setting the MONADO_HEADER_PATH manually:")
        print(f"MONADO_HEADER_PATH=/home/user/monado/src/xrt/targets/libmonado/monado.h ./{parser.prog}")
        return

    if args.focused:
        m.set_focused(args.focused)
    if args.primary:
        m.set_primary(args.primary)
    if args.input:
        m.toggle_io(args.input)

    m.update_clients()

    print(f"Clients: {m.client_count}")
    for x in range(m.client_count):
        c = m.snapshot_client(x)
        print(f"\tid: {c.ident:4d}, primary: {c.primary:d}, active: {c.active:d}, "
              f"visible: {c.visible:d}, focused: {c.focused:d}, io: {c.io_active:d}, "
              f"overlay: {c.overlay:d}, name: {c.name}")

    devices = m.get_devices()
    print(f"Devices: {len(devices)}")
    for dev in devices:
        print(f"\tname: {dev.name}, serial: {dev.serial}")

    roles_map = m.get_device_roles()
    print(f"Roles: {len(roles_map)}")
    for role_name, dev_id in roles_map.items():
        print(f"\trole: {role_name},\tdevice-index: {dev_id:4d}")

    m.destroy()


if __name__ == "__main__":
    main()
