#ifndef _QUEUE_
#define _QUEUE_

typedef struct _queue {
  unsigned char	data[4096];
  int	tail;
  int	head;
} queue;

extern queue qin;
extern queue qout;

void qinit(queue *q);
int qsend(queue *q, unsigned char *packet);
int qrecv(queue *q, unsigned char *packet);

#endif

