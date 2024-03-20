# 更新履歴

## 2022/7/30

- huc.exe

  "symbol table overflow" が発生したので NUMMEMB を 256 から 512 に変更

---

## 2022/7/27

- include/pce/huc_gfx.asm

  fade_color() 不具合修正

    - 関数リファレンスにも存在してないし、未完成の非公開関数だった?

---

## 2022/7/22

- huc.exe

  - 関数名の文字数上限を +10 伸ばした。

- pceas.exe

  - .procbank によるバンクオーバーフロー時に表示されるエラー表示を変更

    (旧) Internal error[1]!

    (新) Internal error[1]! (.procbank bank overflow?)

---

## 2022/7/20

- huc.exe

  "symbol table overflow" が発生したので NUMMEMB を 128 から 256 に変更

---

## 2022/7/18

- pceas.exe

  .data セクションに.db, .dw 命令でデータを作成し、
  そのデータのバンクを超えた時、 bank, location counters が更新されないため、
	 - 次のデータラベル開始でも bank, location counters が補正されない
	 - 以降のデータのバンクアドレスが 0x2000 ズレてしまう
  以上の不具合修正のため、.db, .dw 命令が .data セクションで使用された場合、
  bank, location counters を更新するように変更

---

## 2022/7/1

- include/pce/startup.asm

  - irq_del_vsync_handler 実装

  - irq_del_vsync_handler 実装追加に伴い、 irq_add_vsync_handler 戻り値の仕様を変更
    - (旧)	0:登録成功 / 1:登録失敗
    - (新) 0~3:フック番号 / -1:登録失敗

---

## 2022/6/29

- include/pce/malloc.c

  改良、バグ修正

  - malloc でメモリブロックを確保できなかった時 NULL を返すように修正
  - malloc で指定されたサイズの倍のサイズのメモリブロックを確保する不具合を修正
  - 最後に malloc したメモリブロックを free する時、 malloc 前の状態に復帰するように修正
  - RAM領域外を参照してしまう事があったのを修正
  - その他、メモリブロックの分断が発生した時の挙動を変更

- include/pce/huc.inc

  __stwp, __ldwp : %2 に Y レジスタの値を指定する事ができるように拡張

---

## 2022/6/26

- include/pce/startup.asm

  _sys_system_clock 定義

---

## 2022/5/30

- pceas.exe

  - エラーメッセージ変更

    .dw 命令等のオペランド値のオーバーフローエラーのエラーメッセージを変更

    - (旧) Overflow error!
    - (新) (.dw)Operand value overflow error!

---

## 2022/5/27

- pceas.exe

  - ROM使用率の表示を追加

    ```
    BANK  D                             5732/2460
        CODE    $A000-$B663  [5732]
    BANK  E                              918/7274
        CODE    $8000-$8395  [ 918]
                                        ---- ----
                                          84K  36K

                            TOTAL SIZE =      120K(11.7%)
    ```

---

## 2022/5/24

- huc.exe

  - C_DATA_BANK

    C言語でも DATA_BANK に配置される固定データを記述できるように対応

    ```
    C_DATA_BANK char gc_ROM_DATE[] ={
        0x22,0x05,0x24,0x21,0x23,0x43
    };
    ```

    のように記述すると、以下のようなアセンブラコードが出力される。

    ```
            .data
            .page   3
    _gc_ROM_DATE:
            .db     0x22
            .db     0x05
            .db     0x24
            .db     0x21
            .db     0x23
            .db     0x43
            .data
            .bank CONST_BANK
    LL0:

    ```

---

## 2022/5/23

- pceas.exe
  
  - DATA_BANK でページ境界(0x8000)を超えるデータがあった時、DATA と CODE が競合し
    正しくROMデータが出力されないのを修正

- huc.exe
  
  - 構造体の定義数上限(NUMTAG)を増加(10->100)

---

## 2022/5/22

- huc.exe
  
  - __p6code 対応
  
    指定した関数を MMR6 に配置するアセンブラコードを生成する。

- include/pce/startup.asm

  - HAVE_P6CODE 対応
  
    Cコンパイルの結果、 __p6code 関数があった場合、 プリプロセッサ HAVE_P6CODE が定義されたアセンブラコードが生成される。

    この時、以下のコードが生成される。

      - P6CODE_BANK 定義
      - .org $c000
      - リセット起動時、 MMR6 に P6CODE_BANK を設定

- include/pce/library.asm

  - mem_mapdatabank(),mem_mapdatabanks()の不具合修正
  
    ページ3、4のバンク変更する関数だが、P6CODE_BANK を追加した事でページ4、5の変更する処理になってしまった。

---

## 2022/5/21

- include/pce
  
  - HuC(3.99-dshadoff)で削除されたユーザー割り込みライブラリを復活
  - irq_add_vsync_handler(), irq_enable_user(), irq_disable_user()
  - サウンドドライバー SimpleTracker(st)　ライブラリがビルドできるように修正

- include/pce/st.c
  
  - デバッグ表示位置調整。コメント付加。

---

## 2022/5/20

- examples/shmup
  
  - cygwin でビルドできるように修正

---

## 2022/5/19

- pceas.exe

  - 疑似命令 .procbank の実装
  - -dbprn オプションを追加
  - バージョン表示の変更

- huc.exe
  
  - -v を3つ付加すると pceas.exe を -dbprn   を付加して実行するようにした。
  - バージョン表示の変更
