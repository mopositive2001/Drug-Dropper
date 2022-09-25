/*
	Used Technologies
	- DevkitPro  - free homebrew toolchain, https://github.com/devkitPro/libctru
	- wslay      - c websocket implementation, https://github.com/tatsuhiro-t/wslay
	- websock3ds - base code for project, https://github.com/Cruel/websock3ds
	- Thenaya    - NFC reading code, https://github.com/HiddenRamblings/Thenaya/releases

	All of these technologies together allow us to turn the 3ds into an wireless nfc i/o device
*/

#include <citro2d.h>

#include <arpa/inet.h>
#include "nfc.hpp"
#include "ws3ds.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define VERSION "1.0"

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

#define SOC_BUFFERSIZE 0x100000

static u32* socBuffer;

bool service_init() { // Init Wifi Connection
    amInit();
	cfguInit();
    socBuffer = (u32*)memalign(0x1000, SOC_BUFFERSIZE);
    return R_SUCCEEDED(socInit(socBuffer, SOC_BUFFERSIZE));
}

int cardid = -1; // Last Stored Card ID

void on_message(const struct wslay_event_on_msg_recv_arg *arg) {
	if(cardid == -1)
		return;
    if (arg->opcode == 1) { // Text type
        if(arg->msg_length == 13 && strncmp("IWANTYOURCODE", arg->msg, 13) == 0) { // Checks if client asks for code
			char* json = (char*)malloc(15);
			sprintf(json, "{\"cardid\": %d}", cardid);
			ws3ds_send_text(json); // sends code
			free(json);
			cardid = -1; // resets last stored
			printf("Someone recieved my data");
		}
    }
}

int main(int argc, char* argv[]) {
	// Init libs
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);

	// Create screens
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	u32 clrGreen = C2D_Color32(0xFF, 0x00, 0xFF, 0x00);
	u32 clrClear = C2D_Color32(0xFF, 0xD8, 0xB0, 0x68);

	if(!NFC_Init())
		return 0;

	if(!service_init())
		return 0;
	
	bool connected = false;
    bool connecting = false;
	int socket_client = -1;
	int socket_server = create_listener(10101);

	if(socket_server == -1)
		return 0;

	struct in_addr addr = {(in_addr_t) gethostid()};
    printf("Websocket3ds " VERSION "\n");
    printf("Console IP is %s\n", inet_ntoa(addr));
    printf("Press SELECT to exit. Press A to scan for NFC.\n\n");

    ws3ds_set_message_callback(on_message);

	while(aptMainLoop()) {
		// Connect to client if not already connected
        if (!connected && !connecting) {
            connecting = true;
            printf("Waiting for client connection...\n");
        }
        if (connecting) {
            struct sockaddr_in addr;
            socklen_t addrSize = sizeof(addr);

            if (socket_client == -1)
                socket_client = accept(socket_server, (struct sockaddr*)&addr, &addrSize);
            if (socket_client != -1) {
                if (http_handshake(socket_client) == -1) {
                    printf("Websocket handshake failed!\n");
                    close(socket_client);
                    socket_client = -1;
                } else {
                    connecting = false;
                    connected = true;
                    printf("Client connected (%s).\n", inet_ntoa(addr.sin_addr));
                    ws3ds_init(socket_client);
                    ws3ds_send_text("VERSION " VERSION);
                }
            }
        }

		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;

		if(kDown & KEY_A) {
			u8* data = malloc(TAG_SIZE);

			printf("Scanning... Press start to cancel.");
			NFC_FullRead(data, TAG_SIZE);

			u8 numberid = data[20]; // Allows a staggering whole 255 different id's

			printf("Recieved id: %d", (int)numberid);

			cardid = (int)numberid;
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, clrClear);
		C2D_SceneBegin(top);

		/* TODO: Add Interesting Splash Screen */

		C3D_FrameEnd(0);

        if (connected) {
            // Poll events, -1 indicates connection loss
            if (ws3ds_poll() == -1) {
                connected = false;
                close(socket_client);
                socket_client = -1;
            }
        }
	}

	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
