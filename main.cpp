#include <iostream>
#include "def.h"

OPTION gOption = { 0 };

int CheckPacket();

void ShowUsage()
{
  printf( "chktspkt - MPEG-2 TS packet checker\n");
  printf( "usage: chktspkt [オプション]... TSファイル... \n");
  printf( "\n");
  printf( "  -s, --ignore-start n  開始直後の n秒はエラーを無視する\n");
  printf( "  -e, --ignore-end n    終了直前の n秒はエラーを無視する\n");
  printf( "  -b, --base-timed n    ベース時間を設定 n はUnixTime単位の整数値を設定する\n");
  printf( "  -m, --append-entry    開始と終了のデータエントリを追加して出力する(データ加工向け)\n");
  printf( "  -h, --help            この使い方を表示して終了する\n");
  printf( "\n");
}


bool CheckArgs(int argc, char* argv[])
{
    const char* optstring = "s:e:b:jhm";
    const struct option longopts[] = {
        {  "ignore-start",  required_argument,  0, 's' },
        {  "ignore-end",    required_argument,  0, 'e' },
        {  "base-time",     required_argument,  0, 'b' },
        {  "json",          no_argument,        0, 'j' },
        {  "append-entry",  no_argument,        0, 'm' },
        {  "help",          no_argument,        0, 'h' },
        {         0,                 0,         0,  0  },
    };

    int c;
    int longindex = 0;
    while ((c=getopt_long(argc, argv, optstring, longopts, &longindex)) != -1) {
     switch (c) {
     case 's':
       gOption.start_skip = (int)strtol( optarg, NULL, 0); break;
     case 'e':
       gOption.end_skip = (int)strtol( optarg, NULL, 0); break;
     case 'b':
       gOption.base_time = (int64_t)strtol( optarg, NULL, 0); break;
     case 'j':
       gOption.mode_json = 1; break;
     case 'm':
       gOption.add_start_end_entry = 1; break;
     case 'h':
       ShowUsage(); break;
     default :
       ShowUsage(); break;
     }
    }
    if ( argc == optind )
    {
        ShowUsage();
        return false;
    }

    gOption.input_file = argv[optind];

    return true;
}

int main(int argc, char* argv[])
{
    if (!CheckArgs(argc, argv))
    {
        return -1;
    }
    return CheckPacket();
}
