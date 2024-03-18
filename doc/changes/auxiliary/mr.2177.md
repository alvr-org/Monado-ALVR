u/var: Protect tracker access with a mutex.

Solves a race condition that may crash the debug gui if objects are removed using u_var_remove_root.
