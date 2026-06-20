# ─── Asset System ─────────────────────────────────────────────
#  GUID-based asset management with hot-reload support.
#  API:
#    asset_register(path, type.) -> guid
#    asset_guid_to_path(guid.) -> path
#    asset_path_to_guid(path.) -> guid
#    asset_rename(old_path, new_path.) -> bool
#    asset_get_type(guid.) -> "mesh"|"texture"|"shader"|"font"|"script"
#    asset_is_dirty(guid.) -> 1 if changed
#    asset_check_all(.) -> [dirty_guids]
#    asset_watch_start(.)
#    asset_watch_stop(.)
#    asset_save_registry(path.)
#    asset_load_registry(path.)
#    asset_list(.) -> [[guid, path, type], ...]
# ────────────────────────────────────────────────────────────────
