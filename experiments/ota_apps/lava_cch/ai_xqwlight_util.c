// ==================== ai_xqwlight_util.lava ====================
// xqwlight AI - 工具函数

// ==================== 全局变量 ====================

// RC4 状态
int g_rc4State[256];
int g_rc4X, g_rc4Y;

// Shell 排序步长
int g_shellStep[8] = {0, 1, 4, 13, 40, 121, 364, 1093};

// ==================== 工具函数 ====================

int utilMinMax(int min, int mid, int max)
{
    int result;
    result = mid;
    if (result < min) {
        result = min;
    }
    if (result > max) {
        result = max;
    }
    return result;
}

void shellSort(long mvs[], int vls[], int from, int to)
{
    int stepLevel, step;
    int i, j;
    long mvBest;
    int vlBest;

    stepLevel = 1;
    while (g_shellStep[stepLevel] < to - from) {
        stepLevel++;
    }
    stepLevel--;

    while (stepLevel > 0) {
        step = g_shellStep[stepLevel];
        i = from + step;
        while (i < to) {
            mvBest = mvs[i];
            vlBest = vls[i];
            j = i - step;
            while (j >= from) {
                if (vlBest <= vls[j]) {
                    break;
                }
                mvs[j + step] = mvs[j];
                vls[j + step] = vls[j];
                j -= step;
            }
            mvs[j + step] = mvBest;
            vls[j + step] = vlBest;
            i++;
        }
        stepLevel--;
    }
}

int binarySearch(int vl, int vls[], int from, int to)
{
    int low, high, mid;
    low = from;
    high = to - 1;

    while (low <= high) {
        mid = (low + high) / 2;
        if (vls[mid] < vl) {
            low = mid + 1;
        } else {
            if (vls[mid] > vl) {
                high = mid - 1;
            } else {
                return mid;
            }
        }
    }
    return -1;
}

// ==================== RC4 随机数 ====================

void rc4Swap(int i, int j)
{
    int t;
    t = g_rc4State[i];
    g_rc4State[i] = g_rc4State[j];
    g_rc4State[j] = t;
}

void rc4Init(char key[], int keyLen)
{
    int i, j;
    int k;

    g_rc4X = 0;
    g_rc4Y = 0;

    for (i = 0; i < 256; i++) {
        g_rc4State[i] = i;
    }

    j = 0;
    for (i = 0; i < 256; i++) {
        k = key[i % keyLen] & 0xff;
        j = (j + g_rc4State[i] + k) & 0xff;
        rc4Swap(i, j);
    }
}

int rc4NextByte()
{
    int t;
    int result;

    g_rc4X = (g_rc4X + 1) & 0xff;
    g_rc4Y = (g_rc4Y + g_rc4State[g_rc4X]) & 0xff;
    rc4Swap(g_rc4X, g_rc4Y);
    t = (g_rc4State[g_rc4X] + g_rc4State[g_rc4Y]) & 0xff;
    result = g_rc4State[t] & 0xff;
    return result;
}

long rc4NextLong()
{
    int n0, n1, n2, n3;
    long result;

    n0 = rc4NextByte() & 0xff;
    n1 = rc4NextByte() & 0xff;
    n2 = rc4NextByte() & 0xff;
    n3 = rc4NextByte() & 0xff;
    result = n0 + ((long)n1 << 8) + ((long)n2 << 16) + ((long)n3 << 24);
    return result;
}
