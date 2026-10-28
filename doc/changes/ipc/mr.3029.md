Fix IPC server shutdown hangs and repeated joins by stopping all client loops
before joining, preventing late startup from reviving stopped clients, and
cancelling and disconnecting blocking Windows named-pipe reads.
