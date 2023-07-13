#!/usr/bin/env luajit
-- Copyright 2020-2023, Collabora, Ltd.
-- SPDX-License-Identifier: BSL-1.0
-- Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>

local ffi = require("ffi")

ffi.cdef[[
typedef enum mnd_result
{
	MND_SUCCESS = 0,
	MND_ERROR_INVALID_VERSION = -1,
	MND_ERROR_INVALID_VALUE = -2,
	MND_ERROR_CONNECTING_FAILED = -3,
	MND_ERROR_OPERATION_FAILED = -4,
} mnd_result_t;

typedef enum mnd_client_flags
{
	MND_CLIENT_PRIMARY_APP = (1u << 0u),
	MND_CLIENT_SESSION_ACTIVE = (1u << 1u),
	MND_CLIENT_SESSION_VISIBLE = (1u << 2u),
	MND_CLIENT_SESSION_FOCUSED = (1u << 3u),
	MND_CLIENT_SESSION_OVERLAY = (1u << 4u),
	MND_CLIENT_IO_ACTIVE = (1u << 5u),
} mnd_client_flags_t;

typedef struct mnd_root mnd_root_t;

void
mnd_api_get_version(uint32_t *out_major, uint32_t *out_minor, uint32_t *out_patch);

mnd_result_t
mnd_root_create(mnd_root_t **out_root);

void
mnd_root_destroy(mnd_root_t **root_ptr);

mnd_result_t
mnd_root_update_client_list(mnd_root_t *root);

mnd_result_t
mnd_root_get_number_clients(mnd_root_t *root, uint32_t *out_num);

mnd_result_t
mnd_root_get_client_id_at_index(mnd_root_t *root, uint32_t index, uint32_t *out_client_id);

mnd_result_t
mnd_root_get_client_name(mnd_root_t *root, uint32_t client_id, const char **out_name);

mnd_result_t
mnd_root_get_client_state(mnd_root_t *root, uint32_t client_id, uint32_t *out_flags);

mnd_result_t
mnd_root_set_client_primary(mnd_root_t *root, uint32_t client_id);

mnd_result_t
mnd_root_set_client_focused(mnd_root_t *root, uint32_t client_id);

mnd_result_t
mnd_root_toggle_client_io_active(mnd_root_t *root, uint32_t client_id);
]]

local status, lib = pcall(ffi.load, "libmonado.so")
if not status then
    print("Could not find an installed libmonado.so in your path.")
    print("Add the Monado build directory to your LD_LIBRARY_PATH:")
    print(string.format("LD_LIBRARY_PATH=/home/user/monado/build/src/xrt/targets/libmonado/ %s", arg[0]))
    os.exit(1)
end

-- Parse arguments
local args = {...}
for i=1, #args, 1 do
    if args[i] == "-f" or args[i] == "--focused" then
        args_focused = tonumber(args[i+1])
    elseif args[i] == "-p" or args[i] == "--primary" then
        args_primary = tonumber(args[i+1])
    elseif args[i] == "-i" or args[i] == "--input" then
        args_input = tonumber(args[i+1])
    end
end

-- Using ** doesn't work here, it hits an assertion in moando due being NULL
local root_ptr = ffi.new("mnd_root_t*[1]")

function create_root()
    local ret = lib.mnd_root_create(root_ptr)
    if ret ~= 0 then
        error("Could not create root")
    end
    return root_ptr[0]
end

local root = create_root()

function update_clients()
    local ret = lib.mnd_root_update_client_list(root)
    if ret ~= 0 then
        error("Could not update clients")
    end
end

function get_client_count()
    client_count_ptr = ffi.new("uint32_t[1]")
    ret = lib.mnd_root_get_number_clients(root, client_count_ptr)
    if ret ~= 0 then
        error("Could not get number of clients")
    end
    return client_count_ptr[0]
end

function get_client_id_at_index(index)
    client_id_ptr = ffi.new("uint32_t[1]")
    local ret = lib.mnd_root_get_client_id_at_index(root, index, client_id_ptr)
    if ret ~= 0 then
        error("Could not get client id at index.")
    end
    return client_id_ptr[0]
end

function get_client_name(client_id)
    name_ptr = ffi.new("const char*[1]")
    local ret = lib.mnd_root_get_client_name(root, client_id, name_ptr)
    if ret ~= 0 then
        error("Could not get client name.")
    end
    return name_ptr[0]
end

function get_client_flags(client_id)
    flags_ptr = ffi.new("uint32_t[1]")
    local ret = lib.mnd_root_get_client_state(root, client_id, flags_ptr)
    if ret ~= 0 then
        error("Could not get client flags.")
    end
    return flags_ptr[0]
end

function set_primary(client_id)
    local ret = lib.mnd_root_set_client_primary(root, client_id)
    if ret ~= 0 then
        error("Failed to set primary client to client id", client_id)
    end
end

function set_focused(client_id)
    ret = lib.mnd_root_set_client_focused(root, client_id)
    if ret ~= 0 then
        error("Failed to set focused client to client id", client_id)
    end
end

function toggle_io(client_id)
    ret = lib.mnd_root_toggle_client_io_active(root, client_id)
    if ret ~= 0 then
        error("Failed to toggle io for client client id", client_id)
    end
end

if args_primary then
    set_primary(args_primary)
end
if args_focused then
    set_focused(args_focused)
end
if args_input then
    toggle_io(args_input)
end

update_clients(root)
print("Client count:", get_client_count())

for i = 0, get_client_count() - 1 do
    local client_id = get_client_id_at_index(i)
    local name = get_client_name(client_id)
    local flags = get_client_flags(client_id)
    local primary = bit.band(flags, lib.MND_CLIENT_PRIMARY_APP) ~= 0
    local active = bit.band(flags, lib.MND_CLIENT_SESSION_ACTIVE) ~= 0
    local visible = bit.band(flags, lib.MND_CLIENT_SESSION_VISIBLE) ~= 0
    local focused = bit.band(flags, lib.MND_CLIENT_SESSION_FOCUSED) ~= 0
    local overlay = bit.band(flags, lib.MND_CLIENT_SESSION_OVERLAY) ~= 0
    local io_active = bit.band(flags, lib.MND_CLIENT_IO_ACTIVE) ~= 0
    print(string.format("id: %d primary: %5s active: %5s visible: %5s focused: %5s io: %5s overlay: %5s name: %s",
          client_id, tostring(primary), tostring(active), tostring(visible), tostring(focused),
          tostring(io_active), tostring(overlay), ffi.string(name)))
end

lib.mnd_root_destroy(root_ptr)
