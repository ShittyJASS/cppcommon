#pragma once

#define BUFFERW g_buffer
#define BUFFERA ((char*)g_buffer)
#define BUFFERW_SIZE 4096
#define BUFFERA_SIZE (BUFFERW_SIZE*2)

#ifdef UNICODE
#define BUFFER BUFFERW
#define BUFFER_SIZE BUFFERW_SIZE
#else
#define BUFFER BUFFERA
#define BUFFER_SIZE BUFFERA_SIZE
#endif

static wchar_t BUFFERW[BUFFERW_SIZE];
