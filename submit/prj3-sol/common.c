#include "common.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

/** Read header line from in and fill in hdr */
void
read_header(Hdr *hdr, FILE *in)
{
  char buf[MAX_HDR_LEN];
  fgets(buf, MAX_HDR_LEN, in);
  // fprintf(stderr,"buffer here : %s\n ", buf);
  // fprintf(stderr,"%s\n",buf);
  TRACE("pid = %ld, buf = %s", (long)getpid(), buf);
  assert(buf[strlen(buf)] == '\0');
  if (hdr->hdrType == CLIENT_HDR) {
    int cmdType;
    sscanf(buf, "%d %d %zu %zu", &cmdType, &hdr->count, &hdr->nTopics,
           &hdr->nBytes);
    hdr->cmdType = cmdType;
    // fprintf(stderr,"sccanf %d %d %zu %zu\n", &cmdType, &hdr->count, &hdr->nTopics,
          //  &hdr->nBytes);
  }
  else {
    int serverStatus;
    // fprintf(stderr,"buffer here ");
    // fprintf(stderr,"%s\n",buf);
    sscanf(buf, "%d %zu", &serverStatus, &hdr->nBytes);
    hdr->status = serverStatus;
  }
}

/** write header line for hdr to stream out and flush it. */
void
write_header(const Hdr *hdr, FILE *out)
{
  char buf[MAX_HDR_LEN];
  int nBytes =
    hdr->hdrType == CLIENT_HDR
    ? snprintf(buf, MAX_HDR_LEN, "%d %d %zu %zu\n",
               hdr->cmdType, hdr->count, hdr->nTopics, hdr->nBytes)
    : snprintf(buf, MAX_HDR_LEN, "%d %zu\n", hdr->status, hdr->nBytes);
  assert(nBytes < MAX_HDR_LEN);
  TRACE("pid = %ld, nBytes = %d, buf = %s", (long)getpid(), nBytes, buf);
  fwrite(buf, 1, nBytes, out);
  // fprintf(stderr, "pid = %ld, nBytes = %d, buf = %s", (long)getpid(), nBytes, buf);
  fflush(out);
  // fprintf(stderr,"flusshed ");
}
