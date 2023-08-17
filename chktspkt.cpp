#include "def.h"

#include <vector>
#include <queue>
#include <string>
#include <sstream>

#define STAT_SIZE 8192

extern OPTION gOption;

/*
 *  buf にデータの読み込み
 */
int read_buf( ReadBuf *rb ) {
  int n,m ;

  if ( rb->eof == false ) {
    n = rb->tail - rb->curr;
    if( n > 0) {
      memcpy(rb->buf, rb->curr, n);
    }
    m = read(rb->fd, rb->buf+n, rb->bufsize - n);
    if ( m == 0 ) {
      rb->eof = true ;
    } else if ( m < 0 ) {
      fprintf(stderr, "Error: read_buf() %s\n", strerror(errno));
      exit(-1);
    } else {
      n += m;
      rb->total += m;
    }
    rb->curr = rb->buf ;
    rb->tail = rb->buf + n ;
  }
  return n;
}

/*
 *  buf 中のパケットが空か？ ture = 空
 */
bool packetEmpty( ReadBuf *rb )
{
  /* resync の為に 8パケット分を残して buff の補充   */
  if ( ( rb->curr + rb->unit_size * 8 ) > rb->tail )
    read_buf( rb ); 

  if ( rb->eof == false ) return false ;
  if ( rb->tail >= ( rb->curr + rb->unit_size)) return false ;
  return true;
}

/*
 * syncbyte の検索(ファイルの読み込み付き)
 */
unsigned char *resyncRB(ReadBuf *rb )
{
  int i;
  unsigned char *tail;

  while ( packetEmpty( rb ) == false ) {
    if( *rb->curr == 0x47 ){
      tail = rb->curr + rb->unit_size * 8 ;
      if ( rb->tail > tail ) {
        for(i=1;i<8;i++){
          if( *(rb->curr + rb->unit_size*i) != 0x47) {
            break;
          }
        }
        if(i == 8) {
          return rb->curr;
        }
      }
    }
    rb->curr += 1;
  }

  return NULL;
}

/*
 *  PCRの取得(baseのみ)
 */
int64_t getPCR(const TS_HEADER *hdr, const ADAPTATION_FIELD *adapt )
{
  int64_t  ret ;
  if(hdr->transport_error_indicator == 0){
    if( hdr->adaptation_field_control & 0x02) {
      if(adapt->pcr_flag == 1) {
        ret = ( adapt->program_clock_reference >> 9  ) & 0x1ffffffff ;
        return ret;
      }
    }
  }
  return 0;
}

/*
 *  時刻に変換
 */
char *pcr2str( int64_t  pcr  )
{
  int64_t       ext,base1,base2;
  int           hour,min,sec,msec;
  static char   buff[32];

  base1 = pcr;
  base1 = base1 / 90;       // 90kHz * 1000
  base2 = base1 / 1000;     // 90kHz 

  //base1 はミリ秒単位になっている.
  
  hour = base2 / 3600 ;
  min = ( base2 % 3600) / 60;
  sec = base2 % 60 ;
  msec = ( base1 - (( hour * 3600 )+( min * 60 )+ sec) * 1000);
  sprintf(buff, "%02d:%02d:%02d.%03d", hour,min,sec,msec);

  return buff ;
}

int64_t pcrToMilliseconds( int64_t  pcr  )
{
  int64_t       base1;
  base1 = pcr;
  base1 = base1 / 90;       // 90kHz * 1000

  //base1 はミリ秒単位になっている.
  return base1;
}


/*
 * duration の計算 (PCR Wrap-around を考慮)
 */
int64_t durationCalc( int64_t start, int64_t end )
{
  if ( end > start ) {
    return ( end - start );
  } else {
    return ( 0x1ffffffff - start ) + end ;
  }
}

bool isSkipData(int64_t current_pcr, int64_t duration)
{
    int current_seconds = current_pcr / 90000;
    int duration_seconds = duration / 90000;
    if (gOption.start_skip > 0)
    {
        if( current_seconds < gOption.start_skip)
        {
            return true;
        }
    }
    if (gOption.end_skip > 0 )
    {
        if( duration_seconds - gOption.end_skip < current_seconds )
        {
            return true;
        }
    }
    return false;
}

bool verbose(VERBOSE_DATA *vd, int64_t pcr, int pcc, const char *type, int pid)
{
    printf("%4d %12s %10d  0x%04x  %-12s\n",
           vd->lineC, pcr2str(pcr), pcc, pid, type);
    return false;
}

struct TimebaseLogInfo
{
    int64_t offset_seconds;
    int error;
    int drop;
    int scramble;

    TimebaseLogInfo() {
        offset_seconds = 0;
        error = 0;
        drop = 0;
        scramble = 0;
    }

    std::string GetTimestampString() const
    {
        int hours = offset_seconds / 3600;
        int min = (offset_seconds % 3600) / 60;
        int sec = offset_seconds % 60;

        char buf[64]={0};
        sprintf(buf, "%02d:%02d:%02d", hours, min, sec);
        
        return std::string(buf);
    }
};

struct PacketErrorInfo
{
    uint64_t error;
    uint64_t drop;
    uint64_t scramble;

    PacketErrorInfo() : error(0), drop(0), scramble(0) { }
};
struct PIDSummaryInfo
{
    int pid;
    uint64_t total;
    PacketErrorInfo summary;

    PIDSummaryInfo() : pid(0), total(0), summary() { }
};

struct SummaryInfo
{
    int64_t duration;
    PacketErrorInfo summary;
    std::vector<PIDSummaryInfo> pids;

    SummaryInfo() : duration(0), summary() { }
};

void  OutputStdout(std::vector<TimebaseLogInfo>& logs, const SummaryInfo& summaryInfo)
{
    for(const auto& log : logs)
    {
        std::string timestamp = log.GetTimestampString();

        if (gOption.base_time > 0)
        {
            int64_t unixtime = log.offset_seconds + gOption.base_time;
            tm* t = localtime( &unixtime);
            char buf[256] = { 0 };
            strftime(buf, 256, "%Y-%m-%dT%H:%M:%S", t);
            timestamp = buf;
        }
        fprintf(stdout, "%12s  E:%-5d D:%-5d S:%-5d\n", 
            timestamp.c_str(), log.error, log.drop, log.scramble);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "--------\n");
    fprintf(stdout, "  Duration: %s\n", pcr2str(summaryInfo.duration));
    fprintf(stdout, "  Drop:  %ld\n", summaryInfo.summary.drop);
    fprintf(stdout, "  Error: %ld\n", summaryInfo.summary.error);
    fprintf(stdout, "  Scram:  %ld\n", summaryInfo.summary.scramble);
    fprintf(stdout, "--------\n");

    fprintf(stdout, "  %6s %12s %12s %12s %12s\n", "PID", "Total", "Drop", "Error", "Scramble");
    for(const auto& pid : summaryInfo.pids)
    {
        fprintf(stdout, "  0x%04x %12ld %12ld %12ld %12ld\n",
            pid.pid, pid.total, pid.summary.drop, pid.summary.error, pid.summary.scramble);
    }

    fflush(stdout);
}

void OutputJsonFormat(std::vector<TimebaseLogInfo>& logs, const SummaryInfo& summaryInfo)
{
    fprintf(stdout, "{\n");
    fprintf(stdout, "  \"timebased_log\" : [\n");
    int entry_count  = logs.size();
    int entry_index = 0;
    for(const auto& log : logs)
    {
        std::string timestamp = log.GetTimestampString();

        if (gOption.base_time > 0)
        {
            int64_t unixtime = log.offset_seconds + gOption.base_time;
            tm* t = localtime( &unixtime);
            char buf[256] = { 0 };
            strftime(buf, 256, "%Y-%m-%dT%H:%M:%S+09:00", t);
            timestamp = buf;
        }

        std::stringstream ss;
        ss << "    { ";
        ss << "\"timestamp\" : \"" << timestamp << "\"";
        ss << ", ";
        ss << "\"error\" : " << log.error;;
        ss << ", ";
        ss << "\"drop\" : " << log.drop;
        ss << ", ";
        ss << "\"scramble\" : " << log.scramble;

        ss << "}";

        entry_index++;
        if (entry_index != entry_count)
        {
            ss << ",";
        }

        std::string json_entry = ss.str();
        fprintf(stdout, "%s\n", json_entry.c_str());
    }
    fprintf(stdout, "  ],\n");

    // Summary 情報を出力.
    fprintf(stdout, "  \"summary\" : {\n");
    fprintf(stdout, "    \"drop\" : %ld,\n", summaryInfo.summary.drop);
    fprintf(stdout, "    \"error\" : %ld,\n", summaryInfo.summary.error);
    fprintf(stdout, "    \"scramble\" : %ld,\n", summaryInfo.summary.scramble);
    fprintf(stdout, "    \"duration:\": \"%s\",\n", pcr2str(summaryInfo.duration));

    fprintf(stdout, "    \"pids:\": [\n");
    entry_count  = summaryInfo.pids.size();
    entry_index = 0;
    for(const auto& pid : summaryInfo.pids)
    {
        std::stringstream ss;
        ss << "      {";
        ss << "\"pid\" : \"0x" << std::hex << pid.pid << "\", ";
        ss << std::dec;
        ss << "\"total\" : " << pid.total << ", ";
        ss << "\"drop\" : " << pid.summary.drop << ", ";
        ss << "\"error\" : " << pid.summary.error << ", ";
        ss << "\"scramble\" : " << pid.summary.scramble << " ";
        ss << "}";
        entry_index++;
        if (entry_index != entry_count)
        {
            ss << ",";
        }
        fprintf(stdout, "%s\n", ss.str().c_str());
    }
    fprintf(stdout, "    ]\n"); // pids

    fprintf(stdout, "  }\n"); // summary

    fprintf(stdout, "}\n");
    fflush(stdout);
}


int CheckPacket()
{
    int   fd;
    int   pid;
    int   idx = 0;
    int   lcc;
    int   i,m,n;
    int   unit_size;
    int64_t  total;
    int64_t  startPCR = 0;
    int64_t  currPCR = 0;
    int    syncbyte_lost = 0;
    int    pcc = 1;                  /* packet counter */
  
    std::vector<TS_STATUS>         stat;
    TS_HEADER         header;
    ADAPTATION_FIELD  adapt;
    VERBOSE_DATA      vd = { 1,false };

    unsigned char *p;
    unsigned char buf[BUF_SIZE];
    char          text[128];

    fd = open(gOption.input_file, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "file not open(%s)\n",strerror(errno));
        return -1;
    }

    total = lseek64(fd, 0, SEEK_END);
    lseek64(fd, 0, SEEK_SET);

    stat.resize(STAT_SIZE);
    for(auto& v : stat)
    {
        v.pid = i;
        v.last_continuity_counter = -1;
    }

    ReadBuf rb = { fd, buf,sizeof(buf),0,0,0,false,0 };
    n = read_buf( &rb );

    unit_size = select_unit_size(buf, buf+n);
    if(unit_size < 188)
    {
        fprintf(stderr, "Error: syncbyte not found\n");
        return -1;
    }
    rb.unit_size = unit_size ;

    p = resync( rb.curr, rb.tail, unit_size);
    if ( p == NULL ) {
        fprintf(stderr, "Error: syncbyte not found\n");
        return -1;
    } else {
        rb.curr = p ;
    }

    std::queue<INFO_ENTRY> infoEntries;

    while ( packetEmpty( &rb ) == false )
    {
        /* 同期byte のチェック */
        if ( *rb.curr != 0x47 )
        {
            //if (verbose(&vd,currPCR-startPCR , pcc, "syncbyte lost", 0 )== false )
            //{
            //    syncbyte_lost++;
            //}
            p = resyncRB( &rb );
            if ( p == NULL )
                break;
            rb.curr = p;
        }

        /* ヘッダー解析 */
        extract_ts_header(&header, rb.curr);
        if(header.adaptation_field_control & 2)
        {
            extract_adaptation_field(&adapt, rb.curr+4);
        }
        else
        {
            memset(&adapt, 0, sizeof(adapt));
        }

        int64_t tmp = getPCR(&header, &adapt );
        if ( tmp != 0 )
        {
            currPCR = tmp;
            if ( startPCR == 0 ) 
                startPCR = currPCR ;
        }

        pid = header.pid;
        lcc = stat[pid].last_continuity_counter;

        if ( header.transport_error_indicator != 0)
        { // 伝送エラー
            INFO_ENTRY info{};
            info.pcr = currPCR - startPCR;
            info.pid = pid;
            info.packet = pcc;
            info.err_type = Error;
            infoEntries.push(info);
            stat[pid].error += 1;
        }
        else if( (lcc >= 0) && (adapt.discontinuity_counter == 0) )
        {
            if( pid == 0x1fff )
            {
                // null packet - drop count has no mean
                // do nothing
            }
            else if( (header.adaptation_field_control & 0x01) == 0 )
            {
                // no payload : continuity_counter should not increment
                if(lcc != header.continuity_counter  )
                {
                    INFO_ENTRY info{};
                    info.pcr = currPCR - startPCR;
                    info.pid = pid;
                    info.packet = pcc;
                    info.err_type = Drop;
                    infoEntries.push(info);

                    //sprintf( text, "drop (no payload: %d != %d)", lcc, header.continuity_counter );
                    stat[pid].drop += 1;
                }
            }
            else if(lcc == header.continuity_counter )
            { // 再送の可能性
                // has payload and same continuity_counter
                if(memcmp(stat[pid].last_packet, rb.curr, 188) != 0)
                { // 中身が違う
                    // non-duplicate packet
                    sprintf( text, "drop (%d != %d)", lcc+1, header.continuity_counter );

                    INFO_ENTRY info{};
                    info.pcr = currPCR - startPCR;
                    info.pid = pid;
                    info.packet = pcc;
                    info.err_type = Drop;
                    infoEntries.push(info);
                    stat[pid].drop += 1;                    
                }
                else
                { // 中身が同じ -> 再送は 1回まで
                    stat[pid].duplicate_count += 1;
                    if(stat[pid].duplicate_count > 1)
                    {
                        // duplicate packet count exceeds limit (two)
                        sprintf( text, "drop (Abnormal duplicate count)");

                        INFO_ENTRY info{};
                        info.pcr = currPCR - startPCR;
                        info.pid = pid;
                        info.packet = pcc;
                        info.err_type = Drop;
                        infoEntries.push(info);
                        stat[pid].drop += 1;
                    }
                }
            }
            else
            {
                // 通常時
                m = (lcc + 1) & 0x0f;
                if(m != header.continuity_counter)
                {
                    sprintf( text, "drop (%d != %d)", m,header.continuity_counter );
                    INFO_ENTRY info{};
                    info.pcr = currPCR - startPCR;
                    info.pid = pid;
                    info.packet = pcc;
                    info.err_type = Drop;
                    infoEntries.push(info);

                    stat[pid].drop += 1;
                }
                stat[pid].duplicate_count = 0;
            }
        }
        if(header.transport_scrambling_control)
        {
            INFO_ENTRY info{};
            info.pcr = currPCR - startPCR;
            info.pid = pid;
            info.packet = pcc;
            info.err_type = Scramble;
            infoEntries.push(info);
            stat[pid].scrambling += 1;
        }
        stat[pid].last_continuity_counter = header.continuity_counter;

        memcpy(stat[pid].last_packet, rb.curr, 188);
        stat[pid].total += 1;
        rb.curr += unit_size;
        pcc += 1; 
    } 

    int64_t duration_pcr = durationCalc( startPCR, currPCR );
    
    std::vector<TimebaseLogInfo> logs;
    logs.reserve(infoEntries.size() / 2);

    int64_t last_timestamp = 0; // seconds 単位.
    TimebaseLogInfo logInfo;
    while(!infoEntries.empty())
    {
        auto e = infoEntries.front();
        infoEntries.pop();
        if (isSkipData(e.pcr, duration_pcr) )
        {
            continue;
        }
        
        switch(e.err_type)
        {
        case Error:
            logInfo.error += 1; break;
        case Drop :
            logInfo.drop +=1; break;
        case Scramble:
            logInfo.scramble += 1; break;
        }

        if (logInfo.offset_seconds != last_timestamp )
        {
            logs.push_back(logInfo);

            logInfo = TimebaseLogInfo();
            logInfo.offset_seconds = last_timestamp;
        }
        if(last_timestamp == 0)
        {   // 初回.
            logInfo.offset_seconds = pcrToMilliseconds(e.pcr);
            logInfo.offset_seconds /= 1000; // to seconds
        }
        last_timestamp = pcrToMilliseconds(e.pcr);
        last_timestamp /= 1000; // to seconds
       
    }
    if (logInfo.drop > 0 || logInfo.error > 0 || logInfo.scramble > 0)
    {
        logs.push_back(logInfo);
    }
    if (gOption.add_start_end_entry > 0)
    {
        // 開始時と終了のエントリを追加する.
        // ただし各ステータス情報はゼロとしておく. 
        TimebaseLogInfo beginEntry, lastEntry;

        beginEntry.offset_seconds = pcrToMilliseconds(startPCR) / 1000;
        lastEntry.offset_seconds = pcrToMilliseconds(duration_pcr) / 1000;

        logs.insert(logs.begin(), beginEntry);
        logs.insert(logs.end(), lastEntry);
    }

    // サマリ情報を作成.
    SummaryInfo summaryInfo;
    summaryInfo.pids.reserve(STAT_SIZE);
    summaryInfo.duration = duration_pcr;
    for(int i=0;i<STAT_SIZE;++i)
    {
        long packetNo = 0;
        long drop = 0;
        long error = 0;
        long scram = 0;
        if(stat[i].total > 0)
        {
            packetNo += stat[i].total; 
            drop     += stat[i].drop;
            error    += stat[i].error;
            scram    += stat[i].scrambling;

            PIDSummaryInfo summaryPID;
            summaryPID.pid = i;
            summaryPID.total = stat[i].total;
            summaryPID.summary.drop = stat[i].drop;
            summaryPID.summary.error = stat[i].error;
            summaryPID.summary.scramble = stat[i].scrambling;

            summaryInfo.summary.drop += stat[i].drop;
            summaryInfo.summary.error += stat[i].error;
            summaryInfo.summary.scramble += stat[i].scrambling;
            summaryInfo.pids.push_back(summaryPID);
        }
    }

    if (gOption.mode_json == 0)
    {
        OutputStdout(logs, summaryInfo);
    }
    else
    {
        OutputJsonFormat(logs, summaryInfo);
    }

    fflush(stdout);
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
    return 0;
}

