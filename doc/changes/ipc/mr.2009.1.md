all: Add ability to do more complex IPC communication by introducing VLA
functions. These lets us do the marshalling to some extent oursevles, useful
for sending a non-fixed amount of data. Currently only supports non-fixed sized
messages coming back from the server.
