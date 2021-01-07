#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "llrtsp.h"

#define CALLBACK_MAYBE(PARSER, NAME, ...)                                     \
  do {                                                                        \
    llrtsp_settings_t* settings;                                              \
    settings = (llrtsp_settings_t*) (PARSER)->settings;                       \
    if (settings == NULL || settings->NAME == NULL) {                         \
      err = 0;                                                                \
      break;                                                                  \
    }                                                                         \
    err = settings->NAME(__VA_ARGS__);                                        \
  } while (0)

void llrtsp_init(llrtsp_t* parser, llrtsp_type_t type,
                 const llrtsp_settings_t* settings) {
  llrtsp__internal_init(parser);

  parser->type = type;
  parser->settings = (void*) settings;
}


llrtsp_errno_t llrtsp_execute(llrtsp_t* parser, const char* data, size_t len) {
  return llrtsp__internal_execute(parser, data, data + len);
}


void llrtsp_settings_init(llrtsp_settings_t* settings) {
  memset(settings, 0, sizeof(*settings));
}


llrtsp_errno_t llrtsp_finish(llrtsp_t* parser) {
  int err;

  /* We're in an error state. Don't bother doing anything. */
  if (parser->error != 0) {
    return 0;
  }

  switch (parser->finish) {
    case RTSP_FINISH_SAFE_WITH_CB:
      CALLBACK_MAYBE(parser, on_message_complete, parser);
      if (err != HPE_OK) return err;

    /* FALLTHROUGH */
    case RTSP_FINISH_SAFE:
      return HPE_OK;
    case RTSP_FINISH_UNSAFE:
      parser->reason = "Invalid EOF state";
      return HPE_INVALID_EOF_STATE;
    default:
      abort();
  }
}


void llrtsp_pause(llrtsp_t* parser) {
  if (parser->error != HPE_OK) {
    return;
  }

  parser->error = HPE_PAUSED;
  parser->reason = "Paused";
}


void llrtsp_resume(llrtsp_t* parser) {
  if (parser->error != HPE_PAUSED) {
    return;
  }

  parser->error = 0;
}


void llrtsp_resume_after_upgrade(llrtsp_t* parser) {
  if (parser->error != HPE_PAUSED_UPGRADE) {
    return;
  }

  parser->error = 0;
}


llrtsp_errno_t llrtsp_get_errno(const llrtsp_t* parser) {
  return parser->error;
}


const char* llrtsp_get_error_reason(const llrtsp_t* parser) {
  return parser->reason;
}


void llrtsp_set_error_reason(llrtsp_t* parser, const char* reason) {
  parser->reason = reason;
}


const char* llrtsp_get_error_pos(const llrtsp_t* parser) {
  return parser->error_pos;
}


const char* llrtsp_errno_name(llrtsp_errno_t err) {
#define RTSP_ERRNO_GEN(CODE, NAME, _) case HPE_##NAME: return "HPE_" #NAME;
  switch (err) {
    RTSP_ERRNO_MAP(RTSP_ERRNO_GEN)
    default: abort();
  }
#undef RTSP_ERRNO_GEN
}


const char* llrtsp_method_name(llrtsp_method_t method) {
#define RTSP_METHOD_GEN(NUM, NAME, STRING) case RTSP_##NAME: return #STRING;
  switch (method) {
    RTSP_METHOD_MAP(RTSP_METHOD_GEN)
    default: abort();
  }
#undef RTSP_METHOD_GEN
}


void llrtsp_set_lenient(llrtsp_t* parser, int enabled) {
  if (enabled) {
    parser->flags |= F_LENIENT;
  } else {
    parser->flags &= ~F_LENIENT;
  }
}


/* Callbacks */


int llrtsp__on_message_begin(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_message_begin, s);
  return err;
}


int llrtsp__on_url(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_url, s, p, endp - p);
  return err;
}


int llrtsp__on_status(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_status, s, p, endp - p);
  return err;
}


int llrtsp__on_header_field(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_header_field, s, p, endp - p);
  return err;
}


int llrtsp__on_header_value(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_header_value, s, p, endp - p);
  return err;
}


int llrtsp__on_headers_complete(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_headers_complete, s);
  return err;
}


int llrtsp__on_message_complete(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_message_complete, s);
  return err;
}


int llrtsp__on_body(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_body, s, p, endp - p);
  return err;
}


int llrtsp__on_chunk_header(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_chunk_header, s);
  return err;
}


int llrtsp__on_chunk_complete(llrtsp_t* s, const char* p, const char* endp) {
  int err;
  CALLBACK_MAYBE(s, on_chunk_complete, s);
  return err;
}


/* Private */


void llrtsp__debug(llrtsp_t* s, const char* p, const char* endp,
                   const char* msg) {
  if (p == endp) {
    fprintf(stderr, "p=%p type=%d flags=%02x next=null debug=%s\n", s, s->type,
            s->flags, msg);
  } else {
    fprintf(stderr, "p=%p type=%d flags=%02x next=%02x   debug=%s\n", s,
            s->type, s->flags, *p, msg);
  }
}
