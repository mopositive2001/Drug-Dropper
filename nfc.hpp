#pragma once 

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <3ds.h>
#include <3ds/errf.h>
#include <3ds/services/nfc.h>
#include <3ds/services/apt.h>

#define TAG_SIZE 572

#define NTAG_PAGE_SIZE 4
#define NTAG_FAST_READ_PAGE_COUNT 15
#define NTAG_READ_PAGE_COUNT 4
#define NTAG_BLOCK_SIZE NTAG_READ_PAGE_COUNT * NTAG_PAGE_SIZE

#define NFC_TIMEOUT  200 * 1000000

#define CMD_FAST_READ(pagestart, pagecount) {0x3A, pagestart, pagestart+pagecount-1}
#define CMD_READ(pagestart) {0x30, pagestart}
#define CMD_WRITE(pagestart, data) {0xA2, pagestart, data[0], data[1], data[2], data[3]}
#define CMD_AUTH(pwd) {0x1B, pwd[0], pwd[1], pwd[2], pwd[3]}

#define NTAG_PACK {0x80, 0x80, 0x00, 0x00}

#define NTAG_215_LAST_PAGE 0x86

static Result nfc_auth(u8 *PWD);


#define DnfcStartOtherTagScanning nfcStartOtherTagScanning
#define DnfcGetTagState nfcGetTagState
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcStopScanning nfcStopScanning

int NFC_Init() {
    Result ret = nfcInit(NFC_OpType_RawNFC);
    if(R_FAILED(ret)) {
        printf("nfcInit() failed: 0x%08x.\n", (unsigned int)ret);
        return 0;
    }
    return 1;
}

Result NFC_FullRead(u8* data, int datalen) {
    if (datalen < NTAG_PAGE_SIZE * 4) {
		return -1;
	}
	Result ret=0;

	NFC_TagState prevstate, curstate;

	ret = DnfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
	if(R_FAILED(ret)) {
		printf("StartOtherTagScanning() failed: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}
    

	prevstate = NFC_TagState_Uninitialized;
	while (1) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		
		if(kDown & KEY_B) {
			ret = -1;
			printf("Cancelled.\n");
			break;
		}
		
		ret = DnfcGetTagState(&curstate);
		if(R_FAILED(ret)) {
			printf("nfcGetTagState() failed: 0x%08x.\n", (unsigned int)ret);
			DnfcStopScanning();
			return ret;
		}
		
		if(curstate!=prevstate) {
			prevstate = curstate;
			if(curstate==NFC_TagState_InRange) {
				printf("Tag detected.");
				u8 tagdata[TAG_SIZE];
				memset(tagdata, 0, sizeof(tagdata));
				
				printf("Reading tag");
				for(int i=0x00; i<=NTAG_215_LAST_PAGE ;i+=NTAG_FAST_READ_PAGE_COUNT) {
					printf("Reading.");
					u8 cmd[] = CMD_FAST_READ(i, NTAG_FAST_READ_PAGE_COUNT);
					u8 cmdresult[NTAG_FAST_READ_PAGE_COUNT*NTAG_PAGE_SIZE];
					//printbuf("CMD ", cmd, sizeof(cmd));
					memset(cmdresult, 0, sizeof(cmdresult));
					size_t resultsize = 0;
					
					//printf("page start %x end %x\n", cmd[1], cmd[2]);
					resultsize = NTAG_FAST_READ_PAGE_COUNT*NTAG_PAGE_SIZE;
					
					ret = DnfcSendTagCommand(cmd, sizeof(cmd), cmdresult, sizeof(cmdresult), &resultsize, NFC_TIMEOUT);
					if(R_FAILED(ret)) {
						printf("nfcSendTagCommand() failed: 0x%08x.\n", (unsigned int)ret);
						break;
					}
					if (resultsize < NTAG_FAST_READ_PAGE_COUNT * NTAG_PAGE_SIZE) {
						printf("Read size mismatch expected %d got %d.\n", NTAG_FAST_READ_PAGE_COUNT * NTAG_PAGE_SIZE, resultsize);
						ret = -1;
						break;
					}

					int copycount = sizeof(cmdresult);
					if (((i * NTAG_PAGE_SIZE) + sizeof(cmdresult)) > sizeof(tagdata))
						copycount = ((i * NTAG_PAGE_SIZE) + sizeof(cmdresult)) - sizeof(tagdata);
					memcpy(&tagdata[i * NTAG_PAGE_SIZE], cmdresult, copycount);
					//printbuf("result", cmdresult, resultsize); 
				}
                printf("Finished reading!");
				if (ret == 0) {
					memcpy(data, tagdata, sizeof(tagdata));
					//printbuf("result full ", tagdata, AMIIBO_MAX_SIZE);
				}
				break;
			}
		}
	}
    printf("Stopped Scanning.");
	printf("\n");
	DnfcStopScanning();
	return ret;
}