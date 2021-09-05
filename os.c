#include "os.h"
#include "armv7-m.h"
#include "thread.h"

static uint32_t msCounter;

struct Thread threadList[5];

extern char threadStack;

void blinking_green(void);
void blinking_blue(void);
void blinking_red(void);
void terminal(void);

void startOs(void) {
    int i = 1; /* first thread is idle thread */
    threadList[i++] = startThread(blinking_green);
    threadList[i++] = startThread(blinking_blue);
    threadList[i++] = startThread(blinking_red);
    threadList[i++] = startThread(terminal);

    enterOs();
}

void OS_Handler(void) {
    static int activeThread = 0;
    msCounter++;
    int oldThread = activeThread;

    /* a stupid round-robin scheduler */
    activeThread++;
    if (activeThread == 5) activeThread = 0;

    returnToThread(
        &threadList[oldThread].stackPtr,
        threadList[activeThread].stackPtr
    );
}

void yield(void) {
    idle();
}

uint32_t getMsCount(void) {
    return msCounter;
}