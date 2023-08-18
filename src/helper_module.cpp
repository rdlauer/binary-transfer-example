#include "helper.h"

#include <Notecard.h>

struct context
{
	size_t buf_size;
	size_t notecard_binary_offset;
};

static struct context prv_ctx;

int binary_setup(size_t chunk_size, size_t buf_size)
{
	size_t req_size = NoteBinaryRequiredTxBuffer(chunk_size);

	NoteDebugf("Requested size: %u, available size: %u\n", req_size, buf_size);

	if (req_size > buf_size)
	{
		NoteDebug("Requested size is too big\n");
		return -1;
	}
	prv_ctx.buf_size = buf_size;
	prv_ctx.notecard_binary_offset = 0;

	return 0;
}

int binary_send_to_notecard(const void *data, size_t len)
{
	const char *err =
		NoteBinaryTransmit((uint8_t *)data, len, prv_ctx.buf_size, prv_ctx.notecard_binary_offset);

	if (err)
	{
		NoteDebugf("Error sending data: %s\n", err);
		// on an error (e.g. buffer is full)
		// flush what is currently in storage to a web.post request
		binary_send_to_notehub();
		return 1;
	}
	prv_ctx.notecard_binary_offset += len;

	return 0;
}

void binary_send_to_notehub(void)
{
	const size_t RETRY_COUNT = 5;
	size_t attempt = 0;
	for (bool connected = false; !connected && attempt < RETRY_COUNT; ++attempt)
	{
		if (J *rsp = NoteRequestResponse(NoteNewRequest("hub.status")))
		{
			// Ensure the transaction doesn't return an error
			if (NoteResponseError(rsp))
			{
				const char *err = JGetString(rsp, "err");
				NoteDebug(err);
				NoteDebug("\n");
				JDelete(rsp);
			}

			connected = JGetBool(rsp, "connected");
			JDelete(rsp);
		}
		if (!connected)
		{
			delay(60000); // Wait 1 minute before retrying
		}
	}

	// configure the web.post request according to your specifications
	// https://dev.blues.io/notecard/notecard-walkthrough/web-transactions/
	if (J *req = NoteNewRequest("web.post"))
	{
		JAddStringToObject(req, "route", "ImageTestRoute");
		// JAddStringToObject(req, "name", "/notehub-images");
		JAddStringToObject(req, "content", "images/jpeg");
		JAddBoolToObject(req, "binary", true);
		JAddBoolToObject(req, "verify", true);

		if (!NoteRequest(req))
		{
			NoteDebug("Error sending data\n");
			delay(15000);
		}
	}

	prv_ctx.notecard_binary_offset = 0;
	NoteBinaryReset();
}