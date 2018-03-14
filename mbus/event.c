#include <stdint.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "event.h"

static volatile uint32_t msTicks;
static timer_event_t timer_events[MAX_EVENT];

DWORD WINAPI event_handler(LPVOID par) {
	int i;
	timer_event_f pfunc;
	while (1) {
		msTicks = GetTickCount();
		for (i=0; i<MAX_EVENT; i++) {
			if (timer_events[i].pfunc != NULL) {
				if (timer_events[i].cmp_val <= msTicks) {
					int ret;
					pfunc = timer_events[i].pfunc;
					timer_events[i].pfunc = NULL;
					ret = pfunc(timer_events[i].param);
					if (ret) {
						// re-set
						timer_events[i].pfunc = pfunc;
						timer_events[i].cmp_val = (msTicks + ret);
					}
				}
			}
		}
		Sleep(1);
	}
}

void event_init(void) {
	HANDLE hInstance;
	HANDLE hThread;
	DWORD dwThreadId;
	
	msTicks = 0;
	memset(timer_events, 0, sizeof(timer_events));
	// �������������� ����� ��� ����������� ����:
	hInstance = GetModuleHandle(NULL);
	hThread = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			event_handler,          // thread function name
			NULL,                   // argument to thread function 
			0,                      // use default creation flags 
			&dwThreadId);           // returns the thread identifier 
	if (hThread == NULL) {
		MessageBox(NULL, "�� ���� ������� �����!", "������", MB_OK);
		return;
	}
}

int event_set(uint32_t slot, timer_event_f pfunc, uint32_t delay, int param) {
	if (slot >= MAX_EVENT) {
		return 1;
	}
	if (delay < 1) {
		delay = 1;
	}
	timer_events[slot].cmp_val = (msTicks + delay);
	timer_events[slot].pfunc = (timer_event_f)pfunc;
	timer_events[slot].param = param;
	return 0;
}

int event_unset(uint32_t slot) {
	if (slot >= MAX_EVENT) {
		return 1;
	}
	timer_events[slot].pfunc = NULL;
	return 0;
}

void delay_ms(uint32_t msDelay) {
	uint32_t msStart = GetTickCount();
	while ((GetTickCount()-msStart) < msDelay) Sleep(1);
}

uint32_t get_ms_timer(void) {
	return GetTickCount();
}
