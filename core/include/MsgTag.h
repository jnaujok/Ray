#ifndef __MSGTAG_H__
#define __MSGTAG_H__

    // Message tags between a task master and its tracing tasks.
#define MESSAGE_ERROR       0x00

#define MESSAGE_SCENE       0x01    // master -> task: the scene
#define MESSAGE_RAYS        0x02    // master -> task: a batch of rays (empty: stop)
#define MESSAGE_COLOR       0x03    // task -> master: the traced colors
#define MESSAGE_HELLO       0x04    // task -> master: first message on a connection

#endif
