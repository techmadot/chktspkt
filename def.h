#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h> 

typedef struct {                /* read buff */
  int           fd;
  unsigned char *buf;
  int           bufsize;
  int           unit_size;
  unsigned char *curr;
  unsigned char *tail;
  bool          eof;
  long long     total;
} ReadBuf;


typedef struct {

  int             pid;
  int             last_continuity_counter;

  unsigned long     total;
  unsigned long     error;
  unsigned long     drop;
  unsigned long     scrambling;
  unsigned char     last_packet[188];
  int               duplicate_count;

} TS_STATUS;


typedef struct {
  int           sync;
  int           transport_error_indicator;
  int           payload_unit_start_indicator;
  int           transport_priority;
  int           pid;
  int           transport_scrambling_control;
  int           adaptation_field_control;
  int           continuity_counter;
} TS_HEADER;

typedef struct {

  int           adaptation_field_length;
	
  int           discontinuity_counter;
  int           random_access_indicator;
  int           elementary_stream_priority_indicator;
  int           pcr_flag;
  int           opcr_flag;
  int           splicing_point_flag;
  int           transport_private_data_flag;
  int           adaptation_field_extension_flag;

  int64_t       program_clock_reference;
  int64_t       original_program_clock_reference;

  int           splice_countdown;

  int           transport_private_data_length;
	
  int           adaptation_field_extension_length;
  int           ltw_flag;
  int           piecewise_rate_flag;
  int           seamless_splice_flag;
  int           ltw_valid_flag;
  int           ltw_offset;
  int           piecewise_rate;
  int           splice_type;
  int64_t      dts_next_au;
	
} ADAPTATION_FIELD;

typedef struct {                /* verbose data */
  int       lineC;
  bool      snip;
} VERBOSE_DATA;


#define BUF_SIZE 1024 * 32

typedef struct {
  int   start_skip;
  int   end_skip;
  char* input_file;
  int64_t base_time;
  int   mode_json;
  int   add_start_end_entry;
} OPTION;

extern OPTION gOption;

enum ErrorType
{
  Error = 0,
  Drop = 1,
  Scramble = 2,
};
typedef struct {
  int64_t pcr;
  int pid;
  int packet;
  ErrorType err_type;
} INFO_ENTRY;


int select_unit_size(unsigned char *head, unsigned char *tail);
unsigned char *resync(unsigned char *head, unsigned char *tail, int unit_size);
void extract_ts_header(TS_HEADER *dst, unsigned char *packet);
void extract_adaptation_field(ADAPTATION_FIELD *dst, unsigned char *data);


int64_t getPCR(const TS_HEADER *hdr, const ADAPTATION_FIELD *adapt );
bool    packetEmpty( ReadBuf *rb );
bool    skip( int64_t pcr  );
char    *pcr2str( int64_t pcr);
int     read_buf( ReadBuf *rb );
unsigned char *resyncRB(ReadBuf *rb );
void    packetchk(const char *path);
bool    verbose( VERBOSE_DATA *vd, int64_t pcr, int pcc, const char *type, int pid );
int64_t durationCalc( int64_t start, int64_t end );
