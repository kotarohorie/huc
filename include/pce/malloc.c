/* simple malloc()/free() implementation
   Based on the snes-sdk implementation which has in turn been adapted
   from http://www.flipcode.com/archives/Simple_Malloc_Free_Functions.shtml */

#include "string.h"

/**
 * 変更点
 * - IF_NOT_ENOUGH_RETURN_NULL
 *   malloc で指定されたメモリが確保できない時、 NULL を返すように変更した。
 * - OPTIMIZE_LAST_MALLOC_TO_FREE
 *   最後に malloc したメモリブロックは解放を最適化
 * - __malloc_init() の実行結果を戻り値で返すようにした。
 * - RAM領域外を参照してしまう事があったのを修正
 * - その他、メモリブロックの分断が発生した時の挙動を変更
 */
#define IF_NOT_ENOUGH_RETURN_NULL
#define OPTI_LAST_MALLOC_TO_FREE

#define NULL 0
#define USED 1

struct unit {
    unsigned int size;
};

struct msys_t {
    struct unit* free;
    void * heap_top;        //!< ヒープ先頭アドレス
    unsigned int heap_size; //!< __malloc_init で確保したヒープ総サイズ
#ifdef DEBUG
    struct unit* last;      //!< 最後に malloc したメモリブロック情報ポインタ。 free を実行すると NULL になる。
#endif
};

static struct msys_t msys;
#ifdef DEBUG
char g_malloc_cnt;
char g_compact_cnt;
#endif
/**
 * @param[in]   nsize   実行して確保したいサイズ
 * @return  best    
 */
static struct unit* __compact(unsigned int nsize)
{
    unsigned int bsize, psize;
    struct unit *best;
    struct unit *p;

#ifdef DEBUG
    g_compact_cnt++;
#endif
    p = msys.heap_top;
    best = p;
    bsize = 0;

    while (psize = p->size, psize)
    {
        if (psize & USED)
        {
            if (bsize != 0) {
                best->size = bsize;
                if (bsize >= nsize) {
                    return best;
                }
            }
            // 次のメモリブロックから、やり直す
            bsize = 0;
            best = p = (struct unit *)((unsigned int)p + (psize & ~USED));
        }
        else
        {
            bsize += psize;
            p = (struct unit *)((unsigned int)p + psize);
        }

        // 確保したヒープ領域外になったらヌケ
        if (p >= (msys.heap_top + msys.heap_size)) break;
    }
    // 最後に見つかったメモリブロックが一番大きいとは限らないが
    // メモリブロックを返す。
    best->size = bsize;
    return best;
}
/**
 * @param[in]   ptr    malloc で確保したメモリブロックポインタ
 */
void free(void *ptr)
{
    if (ptr)
    {
        struct unit *p;
#ifdef DEBUG
        msys.last = NULL;
#endif
        p = (struct unit *)((unsigned int)ptr - sizeof(struct unit));
        p->size &= ~USED;
#ifdef OPTI_LAST_MALLOC_TO_FREE
        if ((unsigned int)p + p->size == msys.free)
        {
            p->size += msys.free->size;
            msys.free = p;
        }
#endif
    }
}
/**
 * @param[in]   size    memory allocation size
 */
void *malloc(unsigned int size)
{
    unsigned int fsize;
    struct unit *p;

    if (size == 0) return NULL;
#ifdef DEBUG
    g_malloc_cnt++;
#endif

    // size が unit 含めて4byte境界になるよう補正する.
    size += 3 + sizeof(struct unit);
    size >>= 2;
    size <<= 2;

#ifdef IF_NOT_ENOUGH_RETURN_NULL
    if (size > msys.free->size)
#else
    if (msys.free == NULL || size > msys.free->size)
#endif
    {
        msys.free = __compact(size);
        if (msys.free == NULL) return NULL;
    }

    p = msys.free;
    fsize = msys.free->size;

    if (fsize >= size + sizeof(struct unit)) {
        msys.free = (struct unit *)((unsigned int)p + size);
        msys.free->size = fsize - size;
    }
    else
    {
#ifdef IF_NOT_ENOUGH_RETURN_NULL
        return NULL;
#else
        msys.free = NULL;
        size = fsize;
#endif
    }

    p->size = size | USED;

#ifdef DEBUG
    msys.last = p;
#endif
    return (void *)((unsigned int)p + sizeof(struct unit));
}
/**
 * @param[in]   heap    _heap_start
 * @param[in]   len     HEAP_SIZE
 * @return      0:成功 / !0:失敗(確保できる最大サイズ)
 */
unsigned int __malloc_init(void *heap, unsigned int len)
{
    if (((unsigned int)heap + len) >= 0x4000)
    {
        return ((0x4000 - (unsigned int)heap) & 0xFFFC);
    }

    // len の下位2bitを0にする。
    len >>= 2;
    len <<= 2;
    msys.free = msys.heap_top = (struct unit *)heap;
    msys.free->size = msys.heap_size = len;
#ifdef DEBUG
    msys.last = NULL;
    g_malloc_cnt = 0;
    g_compact_cnt = 0;
#endif
    return 0;
}
/**
 * @brief _heap_start から 1024 バイト確保
 */
extern char _heap_start;
unsigned int __malloc_init2(void)
{
    return __malloc_init(&_heap_start, 1024);
}
/**
 * @remarks
 * 使用用途がないため、仕様削除とする。
 * 実行したところでデフラグされるわけでもなく
 * OPTIMIZE_LAST_MALLOC_TO_FREE の実装によって、
 * 一時確保したメモリブロックの解放は確保前の状態に復帰できるようになった。
 */
#if 0
void compact(void)
{
    msys.free = __compact(msys.heap_size);
}
#endif
void *realloc(void *ptr, unsigned int size)
{
    void *p; p = malloc(size);
    memcpy(p, ptr, size); /* this is suboptimal, but not erroneous */
    free(ptr);
    return p;
}
#ifdef DEBUG
void put_malloc_status(char ty)
{
    char tx;
    tx = 0;
    // malloc 実行回数
    put_hex(g_malloc_cnt, 2, tx, ty);
    tx += 2;
    put_char(' ', tx, ty);
    tx++;
    // compact 実行回数 : !=0 だとメモリブロックの分裂が発生している?
    put_hex(g_compact_cnt, 2, tx, ty);
    tx += 2;
    put_char(' ', tx, ty);
    tx++;
    // ワークポインタ : 一意
    put_hex(&msys, 4, tx, ty);
    tx += 4;
    put_char(' ', tx, ty);
    tx++;
    // ヒープ先頭アドレス : 一意
    put_hex(msys.heap_top, 4, tx, ty);
    tx += 4;
    put_char(' ', tx, ty);
    tx++;
    // ヒープ総サイズ : 一意
    put_hex(msys.heap_size, 4, tx, ty);
    tx += 4;
    put_char(' ', tx, ty);
    tx++;
    // 最後にmallocしたメモリブロック情報 : ポインタ
    if (msys.last != NULL)
    {
        put_hex(msys.last, 4, tx, ty);
        tx += 4;
        put_char(' ', tx, ty);
        tx++;
        // 最後にmallocしたメモリブロック情報 : サイズ
        put_hex(msys.last->size, 4, tx, ty);
        tx += 4;
        put_char(' ', tx, ty);
        tx++;
    }
    else
    {
        put_string("NULL ---- ", tx, ty);
        tx += 10;
    }
    // free(残)情報 : ポインタ
    put_hex(msys.free, 4, tx, ty);
    tx += 4;
    put_char(' ', tx, ty);
    tx++;
    // free(残)情報 : サイズ(残メモリバイト数)
    put_hex(msys.free->size, 4, tx, ty);
}
/**
 * @brief malloc 一覧
 * @details
 * malloc で確保されたメモリブロック一覧を表示する。
 * @param[in]   ty  表示開始テキスト座標(y)
 */
void put_malloc_blocks(char ty)
{
    char cnt, tx;
    struct unit *p;
    put_string("MALLOC LIST", 0, ty);
    ty++;
    cnt = 0;
    p = msys.heap_top;
    while (p != msys.free)
    {
        tx = 0;
        // ポインタ
        put_hex(p, 4, tx, ty);
        tx += 4;
        put_char(' ', tx, ty);
        tx++;
        // サイズ
        put_hex(p->size, 4, tx, ty);
        tx += 4;
        put_char(' ', tx, ty);
        tx++;

        p = (struct unit *)((unsigned int)p + (p->size & ~USED));
        cnt++;
        ty++;
    }
}
#endif
