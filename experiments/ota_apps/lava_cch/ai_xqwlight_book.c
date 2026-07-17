// ==================== ai_xqwlight_book.c ====================
// xqwlight opening book support for LavaX

#ifdef LAVA_ESP32
static char g_bookPath[320];

static const char *bookPath()
{
    snprintf(g_bookPath, sizeof(g_bookPath), "%s/LavaData/BOOK.DAT", ExePath);
    return g_bookPath;
}

#define BOOK_PATH bookPath()
#else
#define BOOK_PATH "/LavaData/BOOK.DAT"
#endif
#define BOOK_RECORD_SIZE 8
#define BOOK_MAX_RECORDS 12081

int g_bookRecordCount = BOOK_MAX_RECORDS;
int g_bookInitialized = 0;
int g_bookDebugEnabled = 1;
int g_bookDebugScanLimit = 4;
int g_bookSearchInsertIndex = 0;

char g_bookBuf[8];
long g_bookReadLock;
int g_bookReadMove;
int g_bookReadValue;

char g_bookBackupSquares[256];
long g_bookBackupKey;
long g_bookBackupLock;
int g_bookBackupWhite;
int g_bookBackupBlack;
int g_bookBackupPlayer;
int g_bookBackupMoveNum;
int g_bookBackupDistance;
long g_bookBackupMvList[MAX_MOVE_NUM];
int g_bookBackupPcList[MAX_MOVE_NUM];
long g_bookBackupKeyList[MAX_MOVE_NUM];
int g_bookBackupChkList[MAX_MOVE_NUM];

long bookShiftLock(long lockValue)
{
    if (lockValue < 0) {
        lockValue = lockValue & 0x7fffffff;
        lockValue = lockValue >> 1;
        lockValue = lockValue + 1073741824;
        return lockValue;
    }

    lockValue = lockValue >> 1;
    return lockValue;
}

int bookLockHigh(long lockValue)
{
    int value;

    value = (int)((lockValue >> 16) & 0xffff);
    return value;
}

int bookLockLow(long lockValue)
{
    int value;

    value = (int)(lockValue & 0xffff);
    return value;
}

int bookStateLooksValid()
{
    if (g_bookInitialized != 0 && g_bookInitialized != 1) {
        return 0;
    }
    if (g_bookRecordCount < 0 || g_bookRecordCount > BOOK_MAX_RECORDS) {
        return 0;
    }
    return 1;
}

void bookResetState()
{
    g_bookInitialized = 0;
    g_bookRecordCount = BOOK_MAX_RECORDS;
}

#ifdef LAVA_NATIVE_COMPILED
typedef FILE* book_file_t;
#else
#define book_file_t int
#endif

int bookReadRecordAt(book_file_t fp, int index)
{
    long offset;
    int bytesRead;

    offset = ((long)index) * BOOK_RECORD_SIZE;
    fseek(fp, offset, SEEK_SET);
    bytesRead = fread(g_bookBuf, 1, BOOK_RECORD_SIZE, fp);
    if (bytesRead < BOOK_RECORD_SIZE) {
        return 0;
    }

    g_bookReadLock = (long)(g_bookBuf[0] & 0xff) |
                     ((long)(g_bookBuf[1] & 0xff) << 8) |
                     ((long)(g_bookBuf[2] & 0xff) << 16) |
                     ((long)(g_bookBuf[3] & 0xff) << 24);
    g_bookReadLock = bookShiftLock(g_bookReadLock);
    g_bookReadMove = (g_bookBuf[4] & 0xff) | ((g_bookBuf[5] & 0xff) << 8);
    g_bookReadValue = (g_bookBuf[6] & 0xff) | ((g_bookBuf[7] & 0xff) << 8);
    return 1;
}

int bookFindFirstIndex(book_file_t fp, long targetLock, int recordCount)
{
    int low;
    int high;
    int mid;
    int foundIndex;

    low = 0;
    high = recordCount - 1;
    foundIndex = -1;
    g_bookSearchInsertIndex = 0;
    while (low <= high) {
        mid = (low + high) / 2;
        if (bookReadRecordAt(fp, mid) == 0) {
            g_bookSearchInsertIndex = low;
            return foundIndex;
        }
        if (g_bookReadLock < targetLock) {
            low = mid + 1;
        } else {
            if (g_bookReadLock > targetLock) {
                high = mid - 1;
            } else {
                foundIndex = mid;
                high = mid - 1;
            }
        }
    }
    g_bookSearchInsertIndex = low;
    return foundIndex;
}

void bookInit()
{
    book_file_t fp;
    long fileSize;
    int recordCount;

#ifdef LAVA_NATIVE_COMPILED
    if(sizeof(void*) == 8) {
        printf("64位程序\n");
    } else {
        printf("32位程序\n");
    }
    printf("sizeof(book_file_t)=%d\n", (int)sizeof(book_file_t));
#endif

    if (bookStateLooksValid() == 0) {
        DPRINTF("[BOOK] state reset: init=%d records=%d\n", g_bookInitialized, g_bookRecordCount);
        bookResetState();
    }

    if (g_bookInitialized == 1) {
        if (g_bookDebugEnabled != 0) {
            DPRINTF("[BOOK] bookInit skipped: init=%d records=%d\n", g_bookInitialized, g_bookRecordCount);
        }
        return;
    }

    if (g_bookDebugEnabled != 0) {
        DPRINTF("[BOOK] bookInit path=%s\n", BOOK_PATH);
    }

    fp = fopen(BOOK_PATH, "rb");
    if (fp == 0) {
        DPRINTF("[BOOK] Cannot open %s\n", BOOK_PATH);
        g_bookRecordCount = 0;
        g_bookInitialized = 1;
        return;
    }

    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    if (fileSize <= 0) {
        g_bookRecordCount = 0;
    } else {
        recordCount = (int)(fileSize / BOOK_RECORD_SIZE);
        if (recordCount > BOOK_MAX_RECORDS) {
            recordCount = BOOK_MAX_RECORDS;
        }
        g_bookRecordCount = recordCount;
    }
    fclose(fp);
    g_bookInitialized = 1;
    DPRINTF("[开局库] 初始化完成: 共 %d 条记录\n", g_bookRecordCount);
}

void bookBackupPosition()
{
    int i;

    for (i = 0; i < 256; i++) {
        g_bookBackupSquares[i] = g_pos_squares[i];
    }

    g_bookBackupKey = g_pos_zobristKey;
    g_bookBackupLock = g_pos_zobristLock;
    g_bookBackupWhite = g_pos_vlWhite;
    g_bookBackupBlack = g_pos_vlBlack;
    g_bookBackupPlayer = g_pos_sdPlayer;
    g_bookBackupMoveNum = g_pos_moveNum;
    g_bookBackupDistance = g_pos_distance;

    for (i = 0; i < MAX_MOVE_NUM; i++) {
        g_bookBackupMvList[i] = g_pos_mvList[i];
        g_bookBackupPcList[i] = g_pos_pcList[i];
        g_bookBackupKeyList[i] = g_pos_keyList[i];
        g_bookBackupChkList[i] = g_pos_chkList[i];
    }
}

void bookRestorePosition()
{
    int i;

    for (i = 0; i < 256; i++) {
        g_pos_squares[i] = g_bookBackupSquares[i];
    }

    g_pos_zobristKey = g_bookBackupKey;
    g_pos_zobristLock = g_bookBackupLock;
    g_pos_vlWhite = g_bookBackupWhite;
    g_pos_vlBlack = g_bookBackupBlack;
    g_pos_sdPlayer = g_bookBackupPlayer;
    g_pos_moveNum = g_bookBackupMoveNum;
    g_pos_distance = g_bookBackupDistance;

    for (i = 0; i < MAX_MOVE_NUM; i++) {
        g_pos_mvList[i] = g_bookBackupMvList[i];
        g_pos_pcList[i] = g_bookBackupPcList[i];
        g_pos_keyList[i] = g_bookBackupKeyList[i];
        g_pos_chkList[i] = g_bookBackupChkList[i];
    }
}

void bookBuildMirrorPosition()
{
    int sq;
    int pc;

    positionClear();
    for (sq = 0; sq < 256; sq++) {
        pc = g_bookBackupSquares[sq];
        if (pc != 0) {
            positionAddPiece(MIRROR_SQUARE(sq), pc, 0);
        }
    }

    if (g_bookBackupPlayer == 1) {
        positionChangeSide();
    }
}

long bookProbeCurrentPosition(int mirrorFlag)
{
    long rawLock;
    long targetLock;
    long recordLock;
    long mv;
    book_file_t fp;
    int bytesRead;
    int index;
    int recordCount;
    int scannedCount;
    int move;
    int debugHigh;
    int debugLow;
    int targetHigh;
    int targetLow;
    int firstIndex;

    recordCount = g_bookRecordCount;
    rawLock = g_pos_zobristLock;
    targetLock = bookShiftLock(rawLock);
    targetHigh = bookLockHigh(targetLock);
    targetLow = bookLockLow(targetLock);

    if (g_bookDebugEnabled != 0) {
        DPRINTF("[开局库] 查询局面 (镜像=%d, 回合=%d)\n", mirrorFlag, g_pos_distance);
        DPRINTF("[开局库] 棋库记录=%d 条\n", recordCount);
    }

    fp = fopen(BOOK_PATH, "rb");
    if (fp == 0) {
        DPRINTF("[开局库] 错误: 无法打开棋库文件 %s\n", BOOK_PATH);
        return 0;
    }

    if (recordCount <= 0) {
        fclose(fp);
        if (g_bookDebugEnabled != 0) {
            DPRINTF("[开局库] 未找到匹配 (棋库为空)\n");
        }
        return 0;
    }

    firstIndex = bookFindFirstIndex(fp, targetLock, recordCount);
    if (firstIndex < 0) {
        if (g_bookDebugEnabled != 0) {
            DPRINTF("[开局库] 未找到匹配 (已扫描 %d 条记录)\n", 0);
        }
        fclose(fp);
        return 0;
    }

    if (g_bookDebugEnabled != 0) {
        DPRINTF("[开局库] 开始扫描 (索引=%d, 镜像=%d)\n", firstIndex, mirrorFlag);
    }

    index = firstIndex;
    scannedCount = 0;
    while (index < recordCount) {
        if (bookReadRecordAt(fp, index) == 0) {
            if (g_bookDebugEnabled != 0) {
                DPRINTF("[开局库] 读取失败 (索引=%d)\n", index);
            }
            break;
        }
        recordLock = g_bookReadLock;
        move = g_bookReadMove;

        if (g_bookDebugEnabled != 0 && scannedCount < g_bookDebugScanLimit) {
            DPRINTF("[开局库] 扫描 [%d] 局面ID=%d\n", index, move);
        }

        if (recordLock == targetLock) {
            fclose(fp);
            mv = move;
            if (mirrorFlag != 0) {
                mv = MIRROR_MOVE(mv);
            }
            if (g_bookDebugEnabled != 0) {
                DPRINTF("[开局库] 匹配成功! 走法=%d (索引=%d)\n", (int)mv, index);
            }
            return mv;
        }

        if (recordLock > targetLock) {
            if (g_bookDebugEnabled != 0) {
                DPRINTF("[开局库] 扫描结束 (已超过目标值)\n");
            }
            break;
        }

        index++;
        scannedCount++;
    }

    fclose(fp);
    if (g_bookDebugEnabled != 0) {
        DPRINTF("[开局库] 未找到匹配 (已扫描 %d 条记录)\n", scannedCount);
    }
    return 0;
}

long positionBookMoveSimple()
{
    long mv;
    int legalFlag;

    if (bookStateLooksValid() == 0) {
        if (g_bookDebugEnabled != 0) {
            DPRINTF("[开局库] 状态异常，已重置\n");
        }
        bookResetState();
    }

    if (g_bookInitialized == 0) {
        bookInit();
    }

    if (g_bookRecordCount == 0) {
        if (g_bookDebugEnabled != 0) {
            DPRINTF("[开局库] 跳过查询 (无记录)\n");
        }
        return 0;
    }

    mv = bookProbeCurrentPosition(0);
    if (mv != 0) {
        legalFlag = 0;
        if (positionMakeMove(mv) != 0) {
            positionUndoMakeMove();
            legalFlag = 1;
        }
        if (g_bookDebugEnabled != 0) {
            if (legalFlag != 0) {
                DPRINTF("[开局库] 找到匹配走法 (合法)\n");
            } else {
                DPRINTF("[开局库] 找到匹配走法 (非法，已拒绝)\n");
            }
        }
        if (legalFlag != 0) {
            return mv;
        }
    }

    bookBackupPosition();
    bookBuildMirrorPosition();
    mv = bookProbeCurrentPosition(1);
    bookRestorePosition();

    if (mv != 0) {
        legalFlag = 0;
        if (positionMakeMove(mv) != 0) {
            positionUndoMakeMove();
            legalFlag = 1;
        }
        if (g_bookDebugEnabled != 0) {
            DPRINTF("[BOOK] legal mirror move=%d legal=%d\n", (int)mv, legalFlag);
        }
        if (legalFlag != 0) {
            return mv;
        }
        if (g_bookDebugEnabled != 0) {
            DPRINTF("[BOOK] reject mirror move=%d\n", (int)mv);
        }
    }

    return 0;
}
