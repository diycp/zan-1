/*
  +----------------------------------------------------------------------+
  | Zan                                                                  |
  +----------------------------------------------------------------------+
  | Copyright (c) 2012-2016 Swoole Team <http://github.com/swoole>       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.0 of the Apache license,    |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.apache.org/licenses/LICENSE-2.0.html                      |
  | If you did not receive a copy of the Apache2.0 license and are unable|
  | to obtain it through the world-wide-web, please send a note to       |
  | license@swoole.com so we can mail you a copy immediately.            |
  +----------------------------------------------------------------------+
  | Author: Tianfeng Han  <mikan.tenny@gmail.com>                        |
  +----------------------------------------------------------------------+
*/

#include "swLog.h"
#include "swoole.h"
#include "swPipe.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int swMsgQueue_create(swMsgQueue *q, int blocking, key_t msg_key, long type)
{
    if (!q){
    	return SW_ERR;
    }

    q->ipc_wait = (!blocking)? IPC_NOWAIT:0;
    q->blocking = blocking;
    int msg_id = msgget(msg_key, IPC_CREAT | O_EXCL | 0666);
    if (msg_id < 0)
    {
        swSysError("msgget() failed.");
        return SW_ERR;
    }
    else
    {
        q->msg_id = msg_id;
        q->type = type;
    }

    return SW_OK;
}

void swMsgQueue_free(swMsgQueue *q)
{
    if (q->deleted)
    {
        msgctl(q->msg_id, IPC_RMID, 0);
    }
}

void swMsgQueue_set_blocking(swMsgQueue *q, uint8_t blocking)
{
    q->ipc_wait = blocking? 0 : IPC_NOWAIT;
}

int swMsgQueue_pop(swMsgQueue *q, swQueue_data *data, int length)
{
    int flag = q->ipc_wait;
    long type = data->mtype;

    return msgrcv(q->msg_id, data, length, type, flag);
}

int swMsgQueue_push(swMsgQueue *q, swQueue_data *in, int length)
{
    int ret = -1;
    while (1)
    {
        ret = msgsnd(q->msg_id, in, length, q->ipc_wait);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN)
            {
                swYield();
                continue;
            }
        }

        break;

    }

    return ret;
}

int swMsgQueue_stat(swMsgQueue *q, int *queue_num, int *queue_bytes)
{
    struct msqid_ds stat;
    if (msgctl(q->msg_id, IPC_STAT, &stat) < 0){
    	return SW_ERR;
    }
    else{
    	*queue_num = stat.msg_qnum;
    	*queue_bytes = stat.msg_qbytes;
    	return SW_OK;
    }
}
