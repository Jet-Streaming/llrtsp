#include <stdio.h>
#ifndef LLRTSP__TEST
# include "llrtsp.h"
#else
# define llrtsp_t llparse_t
#endif  /* */

int llrtsp_message_needs_eof(const llrtsp_t* parser);
int llrtsp_should_keep_alive(const llrtsp_t* parser);

int llrtsp__before_headers_complete(llrtsp_t* parser, const char* p,
                                    const char* endp) {
  /* Set this here so that on_headers_complete() callbacks can see it */
  if ((parser->flags & F_UPGRADE) &&
      (parser->flags & F_CONNECTION_UPGRADE)) {
    /* For responses, "Upgrade: foo" and "Connection: upgrade" are
     * mandatory only when it is a 101 Switching Protocols response,
     * otherwise it is purely informational, to announce support.
     */
    parser->upgrade =
        (parser->type == RTSP_REQUEST || parser->status_code == 101);
  } else {
    //parser->upgrade = (parser->method == RTSP_CONNECT);
  }
  return 0;
}


/* Return values:
 * 0 - No body, `restart`, message_complete
 * 1 - CONNECT request, `restart`, message_complete, and pause
 * 2 - chunk_size_start
 * 3 - body_identity
 * 4 - body_identity_eof
 * 5 - invalid transfer-encoding for request
 */
int llrtsp__after_headers_complete(llrtsp_t* parser, const char* p,
                                   const char* endp) {
  int hasBody;

  hasBody = parser->flags & F_CHUNKED || parser->content_length > 0;
  if (parser->upgrade && (/*parser->method == RTSP_CONNECT ||*/
                          (parser->flags & F_SKIPBODY) || !hasBody)) {
    /* Exit, the rest of the message is in a different protocol. */
    return 1;
  }

  if (parser->flags & F_SKIPBODY) {
    return 0;
  } else if (parser->flags & F_CHUNKED) {
    /* chunked encoding - ignore Content-Length header, prepare for a chunk */
    return 2;
  } else if (parser->flags & F_TRANSFER_ENCODING) {
    if (parser->type == RTSP_REQUEST && (parser->flags & F_LENIENT) == 0) {
      /* RFC 7230 3.3.3 */

      /* If a Transfer-Encoding header field
       * is present in a request and the chunked transfer coding is not
       * the final encoding, the message body length cannot be determined
       * reliably; the server MUST respond with the 400 (Bad Request)
       * status code and then close the connection.
       */
      return 5;
    } else {
      /* RFC 7230 3.3.3 */

      /* If a Transfer-Encoding header field is present in a response and
       * the chunked transfer coding is not the final encoding, the
       * message body length is determined by reading the connection until
       * it is closed by the server.
       */
      return 4;
    }
  } else {
    if (!(parser->flags & F_CONTENT_LENGTH)) {
      if (!llrtsp_message_needs_eof(parser)) {
        /* Assume content-length 0 - read the next */
        return 0;
      } else {
        /* Read body until EOF */
        return 4;
      }
    } else if (parser->content_length == 0) {
      /* Content-Length header given but zero: Content-Length: 0\r\n */
      return 0;
    } else {
      /* Content-Length header given and non-zero */
      return 3;
    }
  }
}


int llrtsp__after_message_complete(llrtsp_t* parser, const char* p,
                                   const char* endp) {
  int should_keep_alive;

  should_keep_alive = llrtsp_should_keep_alive(parser);
  parser->finish = RTSP_FINISH_SAFE;

  /* Keep `F_LENIENT` flag between messages, but reset every other flag */
  parser->flags &= F_LENIENT;

  /* NOTE: this is ignored in loose parsing mode */
  return should_keep_alive;
}


int llrtsp_message_needs_eof(const llrtsp_t* parser) {
  if (parser->type == RTSP_REQUEST) {
    return 0;
  }

  /* See RFC 2616 section 4.4 */
  if (parser->status_code / 100 == 1 || /* 1xx e.g. Continue */
      parser->status_code == 204 ||     /* No Content */
      parser->status_code == 304 ||     /* Not Modified */
      (parser->flags & F_SKIPBODY)) {     /* response to a HEAD request */
    return 0;
  }

  /* RFC 7230 3.3.3, see `llrtsp__after_headers_complete` */
  if ((parser->flags & F_TRANSFER_ENCODING) &&
      (parser->flags & F_CHUNKED) == 0) {
    return 1;
  }

  if (parser->flags & (F_CHUNKED | F_CONTENT_LENGTH)) {
    return 0;
  }

  return 1;
}


int llrtsp_should_keep_alive(const llrtsp_t* parser) {
  if (parser->rtsp_major > 0 && parser->rtsp_minor > 0) {
    /* RTSP/1.1 */
    if (parser->flags & F_CONNECTION_CLOSE) {
      return 0;
    }
  } else {
    /* RTSP/1.0 or earlier */
    if (!(parser->flags & F_CONNECTION_KEEP_ALIVE)) {
      return 0;
    }
  }

  return !llrtsp_message_needs_eof(parser);
}
