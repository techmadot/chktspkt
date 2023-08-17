
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
    00:00:15  E:0     D:8     S:633
    00:00:19  E:0     D:7     S:6444
    00:00:20  E:0     D:11    S:6819
    00:00:21  E:0     D:0     S:6480
    00:00:22  E:0     D:0     S:6449
    00:00:23  E:0     D:0     S:6442
    00:00:24  E:0     D:0     S:7072
  ...

--------
  Duration: 00:03:11.372
  Drop:  26
  Error: 0
  Scram:  1718965
--------
     PID        Total         Drop        Error     Scramble
  0x0000         1884            1            0            0
  0x0001           19            0            0            0
  0x0010          187            1            0            0
  0x0011           94            1            0            0
  0x0012         7982            2            0            0
  0x0014           38            0            0            0
  0x0023            2            0            0            0
  0x0024          188            1            0            0
  0x0028            6            0            0            0
  0x0029            6            0            0            0
  ...
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

