
## chktspkt とは

本プログラムは、MPEG-2 TS パケットの健全性をチェックするものです。
@kaikoma-soft 氏が作成された [tspacketchk](https://github.com/kaikoma-soft/tspacketchk) を元に作成しました。

## 特徴

* いつドロップやエラーなどが発生したのかの時刻を調査できる
    * 正確には先頭からのオフセットの時刻ですが、ベース時間の指定によりおよその時刻は判別可能
* オリジナル tspacketchk との違い
    * エラーの発生時刻は全出力
    * ただし先頭および末尾のエラーを無視する設定を追加
    * 他ツール向けに JSON を出力可能
    * 各PIDごとのエラー情報は省略


## インストール方法
下記で /usr/local/bin/chktspkt にインストールします。
   ```
   % mkdir /tmp/chktspkt
   % cd /tmp/chktspkt
   % git clone https://github.com/techmadot/chktspkt.git .
   % make
   % sudo make install
   ```

## 実行方法

   ```
  % chktspkt [オプション]... TSファイル...
   ```

## オプションの説明


#####  -s, --ignore-start n
開始直後の n秒はエラーを無視する

##### -e, --ignore-end n
終了直前の n秒はエラーを無視する

#####  -h, --help
この使い方を表示して終了する

## 実行結果の例

   ```
<<< test.ts >>>

  No  Time          packetNo  pid     type        
   1  00:00:00.00        144  0x0110  error       
   2  00:00:00.00        159  0x0110  drop (7 != 6)
   3  00:00:00.17       1647  0x01f0  drop (5 != 6)
   4  00:02:54.13    1601939  0x0100  error       
   5  00:02:54.13    1601940  0x0100  error       
   6  00:02:54.13    1601941  0x0100  error       
   7  00:02:54.13    1601943  0x0100  error       
   8  00:02:54.13    1601944  0x0100  error       
   9  00:02:54.13    1601948  0x0100  error       
  10  00:02:54.13    1601949  0x0100  error       
  11  00:02:54.13    1601950  0x0100  error       
  12  00:02:54.13    1601951  0x0100  error       
  13  00:02:54.13    1601952  0x0100  error       
  14  00:02:54.13    1601953  0x0100  error       
  15  00:02:54.13    1601954  0x0100  error       
  16  00:02:54.13    1601955  0x0100  error       
...

   pid      packets         drop        error   scrambling
-----------------------------------------------------------
0x0000         2961            0            0            0
0x0011            6            0            0            0
0x0012          926            0            0            0
0x0100      2629336            1          796          778
0x0110        56928            2           21           20
0x0130           13            0            0            0
0x0138          294            0            0            0
0x01f0         5888            2            3            1
0x01ff         5098            0            3            0
0x0300            1            0            1            0
0x0901         2948            0            2            1
-----------------------------------------------------------
            2704399            5          826          800

            drop+error = 831
         syncbyte lost = 1
 PCR Wrap-around check = OK          (start=24:13:08.30, end=24:18:03.08)
              duration = 00:04:54.77 (2704399 packets, 508427286 byte)
            Check Time = 0.1 sec     (3767.12 Mbyte/sec)
   ```


## 動作確認環境


| マシン           | OS                                    |
|------------------|---------------------------------------|
| Rock5 Model B   | Armbian 23.02 jammy                   |


## 連絡先

不具合報告などは、
[GitHub issuse](https://github.com/techmadot/chktspkt/issues)
の方にお願いします。


## ライセンス
このソフトウェアは、Apache License Version 2.0 ライセンスのも
とで公開します。詳しくは LICENSE を見て下さい。


## 謝辞

このソフトウェアは、
 tspacktchk ( https://github.com/kaikoma-soft/tspacketchk )
を基に、作成したものです。<br>
ツールを作成、公開していただき、ありがとうございました。

