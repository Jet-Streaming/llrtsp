#ifndef INCLUDE_LLRTSP_API_H_
#define INCLUDE_LLRTSP_API_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>

typedef llrtsp__internal_t llrtsp_t;
typedef struct llrtsp_settings_s llrtsp_settings_t;

typedef int (*llrtsp_data_cb)(llrtsp_t*, const char *at, size_t length);
typedef int (*llrtsp_cb)(llrtsp_t*);

struct llrtsp_settings_s {
  /* Possible return values 0, -1, `HPE_PAUSED` */
  llrtsp_cb      on_message_begin;

  llrtsp_data_cb on_url;
  llrtsp_data_cb on_status;
  llrtsp_data_cb on_header_field;
  llrtsp_data_cb on_header_value;

  /* Possible return values:
   * 0  - Proceed normally
   * 1  - Assume that request/response has no body, and proceed to parsing the
   *      next message
   * 2  - Assume absence of body (as above) and make `llrtsp_execute()` return
   *      `HPE_PAUSED_UPGRADE`
   * -1 - Error
   * `HPE_PAUSED`
   */
  llrtsp_cb      on_headers_complete;

  llrtsp_data_cb on_body;

  /* Possible return values 0, -1, `HPE_PAUSED` */
  llrtsp_cb      on_message_complete;

  /* When on_chunk_header is called, the current chunk length is stored
   * in parser->content_length.
   * Possible return values 0, -1, `HPE_PAUSED`
   */
  llrtsp_cb      on_chunk_header;
  llrtsp_cb      on_chunk_complete;
};

/* Initialize the parser with specific type and user settings */
void llrtsp_init(llrtsp_t* parser, llrtsp_type_t type,
                 const llrtsp_settings_t* settings);

/* Initialize the settings object */
void llrtsp_settings_init(llrtsp_settings_t* settings);

/* Parse full or partial request/response, invoking user callbacks along the
 * way.
 *
 * If any of `llrtsp_data_cb` returns errno not equal to `HPE_OK` - the parsing
 * interrupts, and such errno is returned from `llrtsp_execute()`. If
 * `HPE_PAUSED` was used as a errno, the execution can be resumed with
 * `llrtsp_resume()` call.
 *
 * In a special case of CONNECT/Upgrade request/response `HPE_PAUSED_UPGRADE`
 * is returned after fully parsing the request/response. If the user wishes to
 * continue parsing, they need to invoke `llrtsp_resume_after_upgrade()`.
 *
 * NOTE: if this function ever returns a non-pause type error, it will continue
 * to return the same error upon each successive call up until `llrtsp_init()`
 * is called.
 */
llrtsp_errno_t llrtsp_execute(llrtsp_t* parser, const char* data, size_t len);

/* This method should be called when the other side has no further bytes to
 * send (e.g. shutdown of readable side of the TCP connection.)
 *
 * Requests without `Content-Length` and other messages might require treating
 * all incoming bytes as the part of the body, up to the last byte of the
 * connection. This method will invoke `on_message_complete()` callback if the
 * request was terminated safely. Otherwise a error code would be returned.
 */
llrtsp_errno_t llrtsp_finish(llrtsp_t* parser);

/* Returns `1` if the incoming message is parsed until the last byte, and has
 * to be completed by calling `llrtsp_finish()` on EOF
 */
int llrtsp_message_needs_eof(const llrtsp_t* parser);

/* Returns `1` if there might be any other messages following the last that was
 * successfully parsed.
 */
int llrtsp_should_keep_alive(const llrtsp_t* parser);

/* Make further calls of `llrtsp_execute()` return `HPE_PAUSED` and set
 * appropriate error reason.
 *
 * Important: do not call this from user callbacks! User callbacks must return
 * `HPE_PAUSED` if pausing is required.
 */
void llrtsp_pause(llrtsp_t* parser);

/* Might be called to resume the execution after the pause in user's callback.
 * See `llrtsp_execute()` above for details.
 *
 * Call this only if `llrtsp_execute()` returns `HPE_PAUSED`.
 */
void llrtsp_resume(llrtsp_t* parser);

/* Might be called to resume the execution after the pause in user's callback.
 * See `llrtsp_execute()` above for details.
 *
 * Call this only if `llrtsp_execute()` returns `HPE_PAUSED_UPGRADE`
 */
void llrtsp_resume_after_upgrade(llrtsp_t* parser);

/* Returns the latest return error */
llrtsp_errno_t llrtsp_get_errno(const llrtsp_t* parser);

/* Returns the verbal explanation of the latest returned error.
 *
 * Note: User callback should set error reason when returning the error. See
 * `llrtsp_set_error_reason()` for details.
 */
const char* llrtsp_get_error_reason(const llrtsp_t* parser);

/* Assign verbal description to the returned error. Must be called in user
 * callbacks right before returning the errno.
 *
 * Note: `HPE_USER` error code might be useful in user callbacks.
 */
void llrtsp_set_error_reason(llrtsp_t* parser, const char* reason);

/* Returns the pointer to the last parsed byte before the returned error. The
 * pointer is relative to the `data` argument of `llrtsp_execute()`.
 *
 * Note: this method might be useful for counting the number of parsed bytes.
 */
const char* llrtsp_get_error_pos(const llrtsp_t* parser);

/* Returns textual name of error code */
const char* llrtsp_errno_name(llrtsp_errno_t err);

/* Returns textual name of RTSP method */
const char* llrtsp_method_name(llrtsp_method_t method);


/* Enables/disables lenient header value parsing (disabled by default).
 *
 * Lenient parsing disables header value token checks, extending llrtsp's
 * protocol support to highly non-compliant clients/server. No
 * `HPE_INVALID_HEADER_TOKEN` will be raised for incorrect header values when
 * lenient parsing is "on".
 *
 * **(USE AT YOUR OWN RISK)**
 */
void llrtsp_set_lenient(llrtsp_t* parser, int enabled);

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif  /* INCLUDE_LLRTSP_API_H_ */
