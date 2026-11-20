#ifndef __MSGTAG_H__
#define __MSGTAG_H__

    // Message tags between a task master and its tracing tasks.
#define MESSAGE_ERROR       0x00

#define MESSAGE_SCENE       0x01    // master -> task: the scene
#define MESSAGE_TILE        0x02    // master -> task: a tile to trace
#define MESSAGE_PIXELS      0x03    // task -> master: a traced tile's pixels
#define MESSAGE_HELLO       0x04    // task -> master: first message on a connection
#define MESSAGE_REQUEST     0x05    // task -> master: ready for a tile
#define MESSAGE_STOP        0x06    // master -> task: stop

#endif
