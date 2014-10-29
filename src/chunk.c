#include "include/chunk.h"

bool enque(chunk* chunk, queue* q)
{
    if(q->size == q->capacity || chunk == NULL) { return false; }
    else
    { 
        q->size ++;
        q->tail = (q->tail + 1) % q->capacity;
        q->elements[q->tail] = chunk;
    }
    return true;
}

//pops the first queued element out of the queue
bool deque(queue* q)
{
    if(q->size <= 0) { return false; }
    else
    { 
        q->size --;
        q->elements[q->head] = NULL; //remove the ptr to the element
        q->head = (q->head + 1) % q->capacity;
    }
    return true;
}

//return the first queued element (if any)
chunk* front(queue* q)
{
    chunk* chunk = NULL;
    if(q->size > 0)
    {
       chunk = q->elements[q->head];
    }
    
    return chunk;
}

//allocate memory for the queue and set all its default values
queue* createQueue(uint16_t max_elements) 
{
    queue* q = NULL;
    q = calloc(1, sizeof(queue));
    q->capacity = max_elements;
    q->size = 0;
    q->head = 0;
    q->tail = -1;
    q->elements = calloc(max_elements, sizeof(chunk*));  

    for(uint64_t i=0; i<max_elements; ++i)
    {
        q->elements[i] = NULL;
    }
    
    return q;
}
