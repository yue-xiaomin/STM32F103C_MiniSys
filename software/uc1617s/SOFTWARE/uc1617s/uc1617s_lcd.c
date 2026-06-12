/**
 * @file    uc1617s_lcd.c
 * @brief   UC1617S LCD 驱动层实现
 *
 * 内存布局 (每字节4像素, LSB first):
 *   _fb[RA][CA]    RA: 0~127     CA: 0~31
 *
 *   D[1:0] = pixel(x%4==0)    ← 最左
 *   D[3:2] = pixel(x%4==1)
 *   D[5:4] = pixel(x%4==2)
 *   D[7:6] = pixel(x%4==3)    ← 最右
 */
#include <stdlib.h>
#include <string.h>
#include "uc1617s_lcd.h"
#include "uc1617s_conf.h"
#include "delay.h"

/* ================================================================
 *  显示缓冲区: 2bit/像素, 4像素/字节, LSB first
 *
 *  布局 (128×96):
 *    lcd_buf[row * LCD_PAGES + page]
 *    byte 内: [1:0]=像素0, [3:2]=像素1, [5:4]=像素2, [7:6]=像素3
 * ================================================================ */
static uint8_t _fb[LCD_HEIGHT][FB_CA_MAX];

/* 当前总线实例 (运行时绑定) */
static const uc1617s_bus_t *g_bus = (void *)0;


/* 刷新控制 */
static uint8_t _flush_enabled = 1;

/* ================================================================
 *  内部辅助: 命令/数据发送 (直接转发到总线层)
 * ================================================================ */

/*------ 底层辅助 ------*/

static inline void _cmd(uint8_t c)        { g_bus->write_cmd(c); }
static inline void _cmd2(uint8_t c, uint8_t p) { g_bus->write_cmd2(c, p); }
static inline void _dat(const uint8_t *d, uint16_t n) { g_bus->write_data(d, n); }

/**
 * 设置 RAM 游标到 (ca, ra)
 *   CA: 字节地址 0~31    RA: 行地址 0~127
 *
 * 命令序列:
 *   UC_SET_CA     (0x00) | CA[4:0]     → 页列地址
 *   UC_SET_RA_LSB (0x60) | RA[3:0]     → 行地址低4位
 *   UC_SET_RA_MSB (0x70) | RA[6:4]     → 行地址高3位
 */
static void _set_cursor(uint8_t ca, uint8_t ra)
{
    _cmd(UC_SET_CA     | (ca & 0x1F));
    _cmd(UC_SET_RA_LSB | (ra & 0x0F));
    _cmd(UC_SET_RA_MSB | ((ra >> 4) & 0x07));
}

/**
 * 2-bit 灰度 → 填满整个字节
 *   GRAY_WHITE(0) → 0x00
 *   GRAY_LIGHT(1) → 0x55
 *   GRAY_DARK(2)  → 0xAA
 *   GRAY_BLACK(3) → 0xFF
 */
static inline uint8_t _gray_fill(uint8_t gray)
{
    uint8_t v = gray & 0x03;
    return v | (v << 2) | (v << 4) | (v << 6);
}

/**
 * @brief  底层: 将 _fb 的指定矩形区域写入 UC1617S RAM
 *
 *  利用窗口编程只传需要更新的字节
 *  例: 8×16 字符 → ca_range=2, 只传 32 字节
 *
 *  不开关显示 (局刷数据量小, 双端口 SRAM 可并行读写)
 */
static void _flush_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (!_flush_enabled) return;    /* 批量模式: 只写 _fb, 不刷 SRAM */
    if (w == 0 || h == 0) return;
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint8_t ca_start = x >> 2;
    uint8_t ca_end   = (uint8_t)((uint16_t)(x + w - 1) >> 2);
    uint8_t ra_end   = y + h - 1;

    if (ca_end >= FB_CA_MAX)  ca_end  = FB_CA_MAX - 1;
    if (ra_end >= LCD_HEIGHT) ra_end  = LCD_HEIGHT - 1;

    uint8_t ca_count = ca_end - ca_start + 1;

    /* 单个操作: 有关显示, 防花屏 */
    _cmd(UC_DC_GRAY_OFF);

    _cmd(UC_SET_WIN_DIS);
    _cmd2(UC_SET_WPC0, ca_start);
    _cmd2(UC_SET_WPC1, ca_end);
    _cmd2(UC_SET_WPP0, y);
    _cmd2(UC_SET_WPP1, ra_end);
//    _cmd(UC_SET_AC | UC_AC_WA_ON | UC_AC_CA_FIRST | UC_AC_RA_INC);
    _cmd(UC_SET_WIN_EN);
    _set_cursor(ca_start, y);

    for (uint8_t row = y; row <= ra_end; row++) {
        _dat(&_fb[row][ca_start], ca_count);
    }

    _cmd(UC_SET_WIN_DIS);
    _cmd(UC_DC_GRAY_ON);
}




/* ================================================================
 *  公共 API
 * ================================================================ */

void uc1617s_gfx_init(const uc1617s_bus_t *bus)
{
    g_bus = bus;

    /* 总线硬件初始化 (含硬件复位) */
    g_bus->init();
    
    /* 硬件/软件复位 */
    _cmd(UC_SYS_RESET);
    delay_ms(50);

    /* ---- APC: 内部升压, 全色 ---- */
    _cmd2(0x31, 0x00);

    /* ---- 温度补偿: -0.05%/°C ---- */
    _cmd(UC_SET_TC | UC_TC_11);

//    /* ---- 电荷泵: 内部 VLCD (9倍泵) ---- */
//    _cmd(UC_SET_PC | UC_PC_INT);

//    /* ---- 面板负载: LCD 9~13nF ---- */
//    _cmd(UC_SET_PANEL_LOAD | UC_PL_9TO13);

    /* ---- 灰度设置 ---- */
    _cmd(UC_SET_GRAY1 | UC_GRAY1_L3);
    _cmd(UC_SET_GRAY2 | UC_GRAY2_L6);
    _cmd(0xEB);
//    _cmd2(0xD2, 0xEB);
    /* ---- 对比度 ---- */
    _cmd2(UC_SET_VBIAS, UC_CONTRAST_DEFAULT);


#if 0   /* ── 方案A: MY=0, SL=0 (推荐) ── */
    _cmd(UC_SET_MAP);              /* 0xC0 正常方向 */
#else   /* ── 方案B: MY=1, SL=0 ── */
    _cmd(UC_SET_MAP | UC_LC_MY);  /* 0xC4 Y镜像 */
#endif

    /* ---- 滚动行: SL = 96 ---- */
    _cmd(UC_SET_SL_LSB);               /* 0x40 | 0x00 → SL[3:0]=0 */
    _cmd(UC_SET_SL_MSB| 0x6);          /* 0x50 | 0x06 → SL[6:4]=6 */
                                       /* SL = 0b110_0000 = 96 */

    /* ---- COM/Partial Display 范围 ---- */
    _cmd2(UC_SET_CEN, LCD_HEIGHT - 1); /* 0xF1, 127: 128行 */
    _cmd2(UC_SET_DST, 0x00);           /* 0xF2, 0:   局部显示起始 */
    _cmd2(UC_SET_DEN, 127);            /* 0xF3, 127: 局部显示结束 */

    /* ---- 先清屏并刷新, 避免闪花屏 ---- */
    uc1617s_clear(GRAY_BLACK);
    uc1617s_refresh();
    
    _cmd(UC_DC_GRAY_ON);  /* 0xAF */
    delay_ms(10);
}

/**
 * 清屏 — 将帧缓冲全部填为指定灰度
 */
void uc1617s_clear(uint8_t gray)
{
    memset(_fb, _gray_fill(gray), sizeof(_fb));

    if (!_flush_enabled) return;    /* 批量模式: 只写 _fb */

    /* 单独调用: 刷 SRAM */
    _cmd(UC_DC_GRAY_OFF);
//    _cmd(UC_SET_AC | UC_AC_WA_ON | UC_AC_CA_FIRST | UC_AC_RA_INC);
    _set_cursor(0, 0);
    _dat(&_fb[0][0], sizeof(_fb));
    _cmd(UC_DC_GRAY_ON);
}

/**
 * 设置单个像素
 *   x: 0 ~ LCD_WIDTH-1    (像素列)
 *   y: 0 ~ LCD_HEIGHT-1   (行)
 *   gray: GRAY_WHITE / GRAY_LIGHT / GRAY_DARK / GRAY_BLACK
 *
 * 映射公式:
 *   CA    = x >> 2          (每字节4像素)
 *   shift = (x & 3) << 1    (每像素2bit)
 */
void uc1617s_set_pixel(uint8_t x, uint8_t y, uint8_t gray)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint8_t ca    = x >> 2;
    uint8_t shift = (x & 0x03) << 1;

    _fb[y][ca] = (_fb[y][ca] & ~(0x03 << shift))
               | ((gray & 0x03) << shift);

    if (!_flush_enabled) return;    /* 批量模式: 只写 _fb */

    /* 单独调用: 刷 SRAM */
    _cmd(UC_DC_GRAY_OFF);
    _cmd(UC_SET_WIN_DIS);
    _cmd2(UC_SET_WPC0, ca);
    _cmd2(UC_SET_WPC1, ca);
    _cmd2(UC_SET_WPP0, y);
    _cmd2(UC_SET_WPP1, y);
//    _cmd(UC_SET_AC | UC_AC_WA_ON | UC_AC_CA_FIRST | UC_AC_RA_INC);
    _cmd(UC_SET_WIN_EN);
    _set_cursor(ca, y);
    _dat(&_fb[y][ca], 1);
    _cmd(UC_SET_WIN_DIS);
    _cmd(UC_DC_GRAY_ON);
}



/**
 * 读取单个像素灰度值
 */
uint8_t uc1617s_get_pixel(uint8_t x, uint8_t y)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return GRAY_BLACK;

    uint8_t ca    = x >> 2;
    uint8_t shift = (x & 0x03) << 1;

    return (_fb[y][ca] >> shift) & 0x03;
}



/**
 * 刷新 — 将帧缓冲一次性写入 SRAM
 *
 * 前提: AC 配置为 WA=1, CA优先, RA+1 (AC[2:0]=001)
 * 命令: UC_SET_AC(0x88) | UC_AC_WA_ON | UC_AC_CA_FIRST | UC_AC_RA_INC
 *       = 0x88 | 0x01 = 0x89
 */
void uc1617s_refresh(void)
{
    _cmd(UC_DC_GRAY_OFF);    
    /* 配置地址自增: 环绕开启, CA先增, RA+1 */
//    _cmd(UC_SET_AC | UC_AC_WA_ON | UC_AC_CA_FIRST | UC_AC_RA_INC);
    /* 游标归零 */
    _set_cursor(0, 0);

    /* 一次性发送 4096 字节 */
    _dat(&_fb[0][0], sizeof(_fb));
    
//    for (uint8_t row = 0; row < LCD_HEIGHT; row++) {
//        _set_cursor(0, row);
//        _dat(&_fb[row][0], FB_CA_MAX);     /* FB_CA_MAX = 32 */
//    } 
    _cmd(UC_DC_GRAY_ON);    
}


/**
 * 批量绘制开始 — 禁止自动刷新, 所有操作只写 _fb
 */
void uc1617s_batch_begin(void)
{
    _flush_enabled = 0;
}

/**
 * 批量绘制结束 — 一次性全屏刷新
 */
void uc1617s_batch_end(void)
{
    _flush_enabled = 1;
    uc1617s_refresh();   /* 只闪一次 */
}


/* ================================================================
 *  图元绘制
 * ================================================================ */

/**
 * Bresenham 直线算法
 *   (x0,y0) → (x1,y1), 坐标用 int 处理越界
 */
void uc1617s_draw_line(int x0, int y0, int x1, int y1, uint8_t gray)
{
    int dx =  abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    /* 用 int 记录包围盒 */
    int bx0 = x0, by0 = y0, bx1 = x0, by1 = y0;

    for (;;) {
        if (x0 >= 0 && x0 < LCD_WIDTH && y0 >= 0 && y0 < LCD_HEIGHT) {
            uint8_t ca = (uint8_t)x0 >> 2, sh = ((uint8_t)x0 & 3) << 1;
            _fb[y0][ca] = (_fb[y0][ca] & ~(3 << sh))
                         | ((gray & 3) << sh);
        }
        /* 更新包围盒 */
        if (x0 < bx0) bx0 = x0; if (x0 > bx1) bx1 = x0;
        if (y0 < by0) by0 = y0; if (y0 > by1) by1 = y0;

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }

    /* 裁剪包围盒 */
    if (bx0 < 0) bx0 = 0;
    if (by0 < 0) by0 = 0;
    if (bx1 >= LCD_WIDTH)  bx1 = LCD_WIDTH  - 1;
    if (by1 >= LCD_HEIGHT) by1 = LCD_HEIGHT - 1;

    _flush_rect((uint8_t)bx0, (uint8_t)by0,
                (uint8_t)(bx1 - bx0 + 1), (uint8_t)(by1 - by0 + 1));
}


/**
 * 矩形边框
 *   (x, y): 左上角    w, h: 宽高
 */
void uc1617s_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray)
{
    if (w == 0 || h == 0) return;
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint8_t x2 = x + w - 1, y2 = y + h - 1;
    if (x2 >= LCD_WIDTH)  x2 = LCD_WIDTH  - 1;   /* ← 加裁剪 */
    if (y2 >= LCD_HEIGHT) y2 = LCD_HEIGHT - 1;

    for (uint8_t i = x; i <= x2; i++) {
        uint8_t sh = (i & 3) << 1;
        _fb[y][i >> 2]  = (_fb[y][i >> 2]  & ~(3 << sh)) | ((gray & 3) << sh);
        _fb[y2][i >> 2] = (_fb[y2][i >> 2] & ~(3 << sh)) | ((gray & 3) << sh);
    }
    for (uint8_t j = y + 1; j < y2; j++) {
        uint8_t sh_l = (x & 3) << 1, sh_r = (x2 & 3) << 1;
        _fb[j][x >> 2]  = (_fb[j][x >> 2]  & ~(3 << sh_l)) | ((gray & 3) << sh_l);
        _fb[j][x2 >> 2] = (_fb[j][x2 >> 2] & ~(3 << sh_r)) | ((gray & 3) << sh_r);
    }

    _flush_rect(x, y, (uint8_t)(x2 - x + 1), (uint8_t)(y2 - y + 1));
}


/**
 * 填充矩形 (字节对齐加速)
 *
 * 处理策略:
 *   ┌────碎片────┼──── 整字节 memset ────┼────碎片────┐
 *   │  逐像素写  │   直接操作 _fb 内存   │  逐像素写  │
 *   └────────────┼──────────────────────┼────────────┘
 *              col_align              col_end_align
 */
void uc1617s_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray)
{
    if (w == 0 || h == 0) return;

    uint8_t gbyte = _gray_fill(gray);
    uint8_t x_end = x + w, y_end = y + h;
    if (x_end > LCD_WIDTH)  x_end = LCD_WIDTH;
    if (y_end > LCD_HEIGHT) y_end = LCD_HEIGHT;

    for (uint8_t row = y; row < y_end; row++) {
        uint8_t col = x;
        while (col < x_end && (col & 0x03) != 0)
            _fb[row][col >> 2] = /* 逐像素写fb, 不走set_pixel避免双重flush */
                (_fb[row][col >> 2] & ~(0x03 << ((col & 3) << 1)))
                | (gray << ((col & 3) << 1)), col++;

        uint8_t ca_s = col >> 2, ca_e = x_end >> 2;
        if (ca_e > ca_s) {
            memset(&_fb[row][ca_s], gbyte, ca_e - ca_s);
            col = ca_e << 2;
        }
        while (col < x_end)
            _fb[row][col >> 2] =
                (_fb[row][col >> 2] & ~(0x03 << ((col & 3) << 1)))
                | (gray << ((col & 3) << 1)), col++;
    }

    _flush_rect(x, y, x_end - x, y_end - y);
}

/**
 * Bresenham 中点画圆
 *   (cx, cy): 圆心    r: 半径
 *   利用 8 对称性, 每步画 8 个点
 */
void uc1617s_draw_circle(int cx, int cy, int r, uint8_t gray)
{
    if (r <= 0) return;
    int x = r, y = 0, d = 1 - r;
    while (x >= y) {
        
    #define SET_P(px,py) do { \
        int _px=(px), _py=(py); \
        if (_px>=0 && _px<LCD_WIDTH && _py>=0 && _py<LCD_HEIGHT) { \
            uint8_t _ca=(uint8_t)_px>>2, _sh=((uint8_t)_px&3)<<1; \
            _fb[_py][_ca] = (_fb[_py][_ca] & ~(3<<_sh))|((gray&3)<<_sh); \
        } } while(0)
        SET_P(cx+x,cy+y); SET_P(cx-x,cy+y);
        SET_P(cx+x,cy-y); SET_P(cx-x,cy-y);
        SET_P(cx+y,cy+x); SET_P(cx-y,cy+x);
        SET_P(cx+y,cy-x); SET_P(cx-y,cy-x);
    #undef SET_P
        
        y++;
        if (d <= 0) { d += 2*y+1; }
        else        { x--; d += 2*(y-x)+1; }
    }
    int fx = cx - r, fy = cy - r;
    if (fx < 0) fx = 0;
    if (fy < 0) fy = 0;
    int fw = 2 * r + 1, fh = 2 * r + 1;
    if (fx + fw > LCD_WIDTH)  fw = LCD_WIDTH  - fx;
    if (fy + fh > LCD_HEIGHT) fh = LCD_HEIGHT - fy;
    if (fw > 0 && fh > 0)
        _flush_rect((uint8_t)fx, (uint8_t)fy, (uint8_t)fw, (uint8_t)fh);
}

/**
 * 填充圆
 *   利用 8 对称性 + 水平扫描线填充
 */
void uc1617s_fill_circle(int cx, int cy, int r, uint8_t gray)
{
    if (r <= 0) return;
    int x = r, y = 0, d = 1 - r;
    while (x >= y) {
        for (int lx = cx-x; lx <= cx+x; lx++) {
            uint8_t px = (uint8_t)lx;
            if (px < LCD_WIDTH) {
                uint8_t py;
                uint8_t ca = px>>2, sh = (px&3)<<1;
                uint8_t gv = (gray & 3) << sh, mask = ~(3 << sh);

                py = (uint8_t)(cy+y);
                if (py < LCD_HEIGHT) _fb[py][ca] = (_fb[py][ca] & mask) | gv;
                py = (uint8_t)(cy-y);
                if (py < LCD_HEIGHT) _fb[py][ca] = (_fb[py][ca] & mask) | gv;
            }
        }
        for (int lx = cx-y; lx <= cx+y; lx++) {
            uint8_t px = (uint8_t)lx;
            if (px < LCD_WIDTH) {
                uint8_t py;
                uint8_t ca = px>>2, sh = (px&3)<<1;
                uint8_t gv = (gray & 3) << sh, mask = ~(3 << sh);

                py = (uint8_t)(cy+x);
                if (py < LCD_HEIGHT) _fb[py][ca] = (_fb[py][ca] & mask) | gv;
                py = (uint8_t)(cy-x);
                if (py < LCD_HEIGHT) _fb[py][ca] = (_fb[py][ca] & mask) | gv;
            }
        }
        y++;
        if (d <= 0) { d += 2*y+1; }
        else        { x--; d += 2*(y-x)+1; }
    }
    
    int fx = cx - r, fy = cy - r;
    if (fx < 0) fx = 0;
    if (fy < 0) fy = 0;
    int fw = 2 * r + 1, fh = 2 * r + 1;
    if (fx + fw > LCD_WIDTH)  fw = LCD_WIDTH  - fx;
    if (fy + fh > LCD_HEIGHT) fh = LCD_HEIGHT - fy;
    if (fw > 0 && fh > 0)
        _flush_rect((uint8_t)fx, (uint8_t)fy, (uint8_t)fw, (uint8_t)fh);
}




#include "uc1617s_font.h"
/*===========================================================================
 * 内部渲染: 只画前景 (透明背景, 不破坏原有内容)
 * 用于: draw_char, draw_string (列式)
 *===========================================================================*/
static void _render_fg(uint8_t x, uint8_t y,
                       const uint8_t *glyph,
                       uint8_t width, uint8_t height,
                       uint8_t gray)
{
    uint8_t pages = (height + 7) >> 3;

    for (uint8_t col = 0; col < width; col++) {
        for (uint8_t page = 0; page < pages; page++) {
            uint8_t bits = glyph[page * width + col];
            if (bits == 0) continue;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (!(bits & (1 << bit))) continue;
                uint8_t row = (page << 3) + bit;
                if (row >= height) break;

                uint8_t px = x + col, py = y + row;
                if (px >= LCD_WIDTH || py >= LCD_HEIGHT) continue;

                uint8_t ca = px >> 2, sh = (px & 3) << 1;
                _fb[py][ca] = (_fb[py][ca] & ~(3 << sh))
                            | ((gray & 3) << sh);
            }
        }
    }
}

/*===========================================================================
 * 列式字符绘制 (高性能, 无背景色)
 *===========================================================================*/
void uc1617s_draw_char(uint8_t x, uint8_t y, char c,
                       uint8_t gray, const uc_font_t *font)
{
    if ((uint8_t)c < font->first ||
        (uint8_t)c >= font->first + font->count) return;

    uint8_t idx = (uint8_t)c - font->first;
    const uint8_t *glyph = font->data + (uint16_t)idx * font->width
                           * ((font->height + 7) >> 3);

    _render_fg(x, y, glyph, font->width, font->height, gray);
    _flush_rect(x, y, font->width, font->height);
}



/*===========================================================================
 * 字符串绘制 (列式, 高性能)
 *===========================================================================*/
void uc1617s_draw_string(uint8_t x, uint8_t y, const char *str,
                         uint8_t gray, const uc_font_t *font)
{
    uint8_t x_start = x;

    while (*str) {
        uint8_t c = (uint8_t)*str;
        if (c >= font->first && c < font->first + font->count) {
            uint8_t idx = c - font->first;
            const uint8_t *glyph = font->data + (uint16_t)idx * font->width
                                   * ((font->height + 7) >> 3);
            _render_fg(x, y, glyph, font->width, font->height, gray);
        }
        x += font->width;
        str++;
    }

    _flush_rect(x_start, y, x - x_start, font->height);
}



/*===========================================================================
 * 整数显示
 *===========================================================================*/
void uc1617s_draw_int(uint8_t x, uint8_t y, int32_t num,
                      uint8_t gray, const uc_font_t *font)
{
    char buf[12];
    uint8_t i = 0;
    uint32_t n;

    if (num < 0) {
        uc1617s_draw_char(x, y, '-', gray, font);
        x += font->width;
        n = -num;
    } else {
        n = num;
    }

    do { buf[i++] = '0' + (n % 10); n /= 10; } while (n > 0);
    while (i > 0) {
        uc1617s_draw_char(x, y, buf[--i], gray, font);
        x += font->width;
    }
}


/*===========================================================================
 * 抗锯齿字符 (4灰度边缘平滑)
 *===========================================================================*/
void uc1617s_draw_char_aa(uint8_t x, uint8_t y, char c,
                          uint8_t fg, uint8_t bg, uint8_t edge,
                          const uc_font_t *font)
{
    if ((uint8_t)c < font->first ||
        (uint8_t)c >= font->first + font->count) return;

    uint8_t idx   = (uint8_t)c - font->first;
    uint8_t pages = (font->height + 7) >> 3;
    const uint8_t *glyph = font->data + (uint16_t)idx * font->width * pages;

    for (uint8_t col = 0; col < font->width; col++) {
        for (uint8_t page = 0; page < pages; page++) {
            uint8_t bits = glyph[page * font->width + col];
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t row = (page << 3) + bit;
                if (row >= font->height) break;

                uint8_t px = x + col, py = y + row;
                if (px >= LCD_WIDTH || py >= LCD_HEIGHT) continue;

                uint8_t gray;
                if (bits & (1 << bit)) {
                    gray = fg;
                } else {
                    uint8_t aa = 0;
                    if (col > 0 && (glyph[page * font->width + col - 1] & (1 << bit))) aa = 1;
                    if (col < font->width - 1 && (glyph[page * font->width + col + 1] & (1 << bit))) aa = 1;
                    if (bit > 0 && (bits & (1 << (bit - 1)))) aa = 1;
                    if (bit < 7 && (bits & (1 << (bit + 1)))) aa = 1;
                    if (!aa) continue;   /* 背景透明 */
                    gray = edge;
                }

                uint8_t ca = px >> 2, sh = (px & 3) << 1;
                _fb[py][ca] = (_fb[py][ca] & ~(3 << sh)) | ((gray & 3) << sh);
            }
        }
    }

    _flush_rect(x, y, font->width, font->height);
}





/*===========================================================================
 *  中文16×16字模渲染 (列式, 交错排列, LSB=顶)
 *
 *  数据布局: data[col*2] = page0(行0~7), data[col*2+1] = page1(行8~15)
 *  取位:     bit0 = 顶行, bit7 = 底行(页内)
 *===========================================================================*/
static void _render_cn_glyph(uint8_t x, uint8_t y,
                              const uint8_t *glyph,
                              uint8_t fg, uint8_t bg)
{
    for (uint8_t col = 0; col < 16; col++) {
        uint8_t lo = glyph[col * 2];
        uint8_t hi = glyph[col * 2 + 1];

        for (uint8_t bit = 0; bit < 8; bit++) {
            uint8_t px = x + col;
            uint8_t ca = px >> 2, sh = (px & 3) << 1;

            uint8_t py0 = y + bit;
            if (px < LCD_WIDTH && py0 < LCD_HEIGHT) {
                uint8_t gray = (lo & (1 << bit)) ? fg : bg;
                _fb[py0][ca] = (_fb[py0][ca] & ~(3 << sh)) | ((gray & 3) << sh);
            }
            uint8_t py1 = y + 8 + bit;
            if (px < LCD_WIDTH && py1 < LCD_HEIGHT) {
                uint8_t gray = (hi & (1 << bit)) ? fg : bg;
                _fb[py1][ca] = (_fb[py1][ca] & ~(3 << sh)) | ((gray & 3) << sh);
            }
        }
    }
}


/*===========================================================================
 * 单个中文字符 (列式, 有背景色)
 *===========================================================================*/
void uc1617s_draw_chinese(uint8_t x, uint8_t y, uint16_t gb2312,
                          uint8_t fg, uint8_t bg,
                          const uc_font_cn_t *font, uint16_t count)
{
    if (x > LCD_WIDTH - 16 || y > LCD_HEIGHT - 16) return;

    uint16_t idx;
    for (idx = 0; idx < count; idx++)
        if (font[idx].code == gb2312) break;
    if (idx >= count) return;

    _render_cn_glyph(x, y, font[idx].data, fg, bg);
    _flush_rect(x, y, 16, 16);
}

/*===========================================================================
 * 中英文混合字符串 (列式, 高性能)
 *
 * ASCII:   用 uc_font_t (列式), 8像素宽
 * 中文:    用 uc_font_cn_t (列式), 16像素宽
 *===========================================================================*/
void uc1617s_draw_string_cn(uint8_t x, uint8_t y, const char *str,
                            uint8_t fg, uint8_t bg,
                            const uc_font_cn_t *cn_font, uint16_t cn_count,
                            const uc_font_t *ascii_font)
{
    uint8_t x_start = x;

    while (*str) {
        if ((uint8_t)*str >= 0x80) {
            /* GB2312 中文 */
            if (x > LCD_WIDTH - 16) { x = 0; y += 16; }
            if (y > LCD_HEIGHT - 16) break;

            uint16_t code = ((uint8_t)*str << 8) | (uint8_t)*(str + 1);
            uint16_t idx;
            for (idx = 0; idx < cn_count; idx++)
                if (cn_font[idx].code == code) break;
            if (idx < cn_count)
                _render_cn_glyph(x, y, cn_font[idx].data, fg, bg);

            x += 16;
            str += 2;
        } else {
            /* ASCII */
            if (x > LCD_WIDTH - ascii_font->width) {
                x = 0; y += ascii_font->height;
            }
            if (y > LCD_HEIGHT - ascii_font->height) break;

            uint8_t c = (uint8_t)*str;
            if (c >= ascii_font->first && c < ascii_font->first + ascii_font->count) {
                uint8_t idx = c - ascii_font->first;
                const uint8_t *glyph = ascii_font->data + (uint16_t)idx * ascii_font->width
                                       * ((ascii_font->height + 7) >> 3);
                _render_fg(x, y, glyph, ascii_font->width, ascii_font->height, fg);
            }

            x += ascii_font->width;
            str += 1;
        }
    }

    _flush_rect(x_start, y, x - x_start, 16);
}


/*===========================================================================
 *  显示原始图像数据 (无头数据)
 *
 *  原理:
 *    数据与 SRAM 格式完全一致, 可直接写入
 *    通过窗口编程只更新图像区域, 不影响屏幕其余部分
 *
 *  流程:
 *    1. 设置窗口边界 (x,y) ~ (x+w-1, y+h-1)
 *    2. 使能窗口编程
 *    3. 写入图像数据
 *    4. 禁用窗口编程
 *===========================================================================*/
void uc1617s_draw_image_raw(uint8_t x, uint8_t y,
                            const uint8_t *data,
                            uint8_t width, uint8_t height)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint8_t w = width, h = height;
    if (x + w > LCD_WIDTH)  w = LCD_WIDTH  - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;

    uint8_t src_bpr = (width + 3) >> 2;

    for (uint8_t row = 0; row < h; row++) {
        const uint8_t *src = &data[row * src_bpr];
        for (uint8_t col = 0; col < w; col++) {
            /* 从源读取像素 */
            uint8_t pixel = (src[col >> 2] >> ((col & 3) << 1)) & 3;

            /* 写入目标 */
            uint8_t dx = x + col;
            uint8_t ca = dx >> 2, sh = (dx & 3) << 1;
            _fb[y + row][ca] = (_fb[y + row][ca] & ~(3 << sh))
                             | (pixel << sh);
        }
    }

    _flush_rect(x, y, w, h);
}




/*===========================================================================
 * 中文/中英文混合显示测试
 *
 * 测试项目:
 *   1. 单个中文字符显示
 *   2. 中文字符串显示
 *   3. 中英文混合显示
 *   4. 不同灰度中文显示
 *   5. 多行换行显示
 *   6. 边界测试
 *===========================================================================*/

void uc1617s_cn_test(void)
{
//    /* ==========================================================
//     *  测试1: 单个中文字符
//     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);

//    /* 逐字显示, 验证每个字模正确 */
//    uc1617s_draw_chinese(0,   0, 0xC4E3, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 你 */
//    uc1617s_draw_chinese(16,  0, 0xBAC3, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 好 */
//    uc1617s_draw_chinese(32,  0, 0xD5E2, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 这 */
//    uc1617s_draw_chinese(48,  0, 0xCAC7, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 是 */
//    uc1617s_draw_chinese(64,  0, 0xD2BB, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 一 */
//    uc1617s_draw_chinese(80,  0, 0xB8F6, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 个 */
//    uc1617s_draw_chinese(96,  0, 0xD2BA, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 液 */

//    uc1617s_draw_chinese(0,  18, 0xBEA7, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 晶 */
//    uc1617s_draw_chinese(16, 18, 0xC6C1, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 屏 */
//    uc1617s_draw_chinese(32, 18, 0xCFD4, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 显 */
//    uc1617s_draw_chinese(48, 18, 0xCABE, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT); /* 示 */

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[1/6] Single CN", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);

//    /* ==========================================================
//     *  测试2: 中文字符串 (draw_string_cn)
//     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);
//    uc1617s_draw_string_cn(0,  0, "你好这是",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(0, 18, "一个液晶屏",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(0, 36, "显示",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[2/6] CN String", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);

//    /* ==========================================================
//     *  测试3: 中英文混合显示
//     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);

//    /* 中文+ASCII 混合 */
//    uc1617s_draw_string_cn(0,  0, "你好Hello",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(0, 18, "温度:25.6C",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(0, 36, "液晶显示OK",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(0, 54, "屏Test123",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[3/6] Mix", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);

//    /* ==========================================================
//     *  测试4: 不同灰度中文显示
//     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);

//    /* 浅灰背景上的深灰中文 */
//    uc1617s_fill_rect(0, 0, 128, 20, GRAY_LIGHT);
//    uc1617s_draw_string_cn(2, 2, "你好这是",
//                           GRAY_BLACK, GRAY_LIGHT,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    /* 深灰背景上的浅灰中文 */
//    uc1617s_fill_rect(0, 22, 128, 20, GRAY_DARK);
//    uc1617s_draw_string_cn(2, 24, "液晶显示",
//                           GRAY_LIGHT, GRAY_DARK,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    /* 黑色背景上的白色中文 */
//    uc1617s_fill_rect(0, 44, 128, 20, GRAY_BLACK);
//    uc1617s_draw_string_cn(2, 46, "一个屏幕",
//                           GRAY_WHITE, GRAY_BLACK,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    /* 白色背景上的黑色中文 (默认) */
//    uc1617s_draw_string_cn(2, 68, "Hello你好",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[4/6] Gray", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);

//    /* ==========================================================
//     *  测试5: 多行混合 + 模拟实际界面
//     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);

//    /* 标题栏 */
//    uc1617s_fill_rect(0, 0, 128, 18, GRAY_DARK);
//    uc1617s_draw_string_cn(2, 1, "液晶显示屏",
//                           GRAY_WHITE, GRAY_DARK,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    /* 内容区 */
//    uc1617s_draw_string_cn(4, 20, "温度:25.6C",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(4, 38, "这是Hello",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(4, 56, "显示OK",
//                           GRAY_DARK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    /* 分隔线 */
//    uc1617s_draw_line(0, 18, 127, 18, GRAY_DARK);
//    uc1617s_draw_line(0, 74, 127, 74, GRAY_DARK);

//    /* 底部状态栏 */
//    uc1617s_fill_rect(0, 76, 128, 12, GRAY_LIGHT);
//    uc1617s_draw_string_cn(4, 78, "屏幕Test",
//                           GRAY_BLACK, GRAY_LIGHT,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    /* 底部提示 */
//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[5/6] UI", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);

//    /* ==========================================================
//     *  测试6: 边界测试
//     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);

//    /* 贴左上角 */
//    uc1617s_draw_chinese(0, 0, 0xC4E3, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT);

//    /* 贴右边 (部分超出, 不应崩溃) */
//    uc1617s_draw_chinese(112, 0, 0xBAC3, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT);

//    /* 贴底边 (部分超出) */
//    uc1617s_draw_chinese(0, 80, 0xD5E2, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT);

//    /* 完全超出 (不显示, 不崩溃) */
//    uc1617s_draw_chinese(200, 0, 0xC4E3, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT);
//    uc1617s_draw_chinese(0, 200, 0xBAC3, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT);

//    /* 中英混合超长行 (测试自动换行) */
//    uc1617s_draw_string_cn(0, 20, "你好这是Hello一个液晶屏Test显示OK",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    /* 缺字测试 (字库中没有的字, 应跳过不显示) */
//    uc1617s_draw_chinese(64, 64, 0xFFFF, GRAY_BLACK, GRAY_WHITE, _cn_font, CN_FONT_COUNT);
//    uc1617s_draw_string_cn(0, 40, "没有的字:",
//                           GRAY_BLACK, GRAY_WHITE,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[6/6] Boundary", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);
/* ==========================================================
     *  测试7: draw_char + draw_int
     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);

//    /* 单字符 */
//    uc1617s_draw_char(0, 0, 'A', GRAY_BLACK, uc_font_8x16);
//    uc1617s_draw_char(10, 0, 'B', GRAY_BLACK, uc_font_8x16);
//    uc1617s_draw_char(20, 0, 'C', GRAY_BLACK, uc_font_8x16);

//    uc1617s_draw_char(0, 18, 'a', GRAY_DARK, uc_font_6x8);
//    uc1617s_draw_char(8, 18, 'b', GRAY_DARK, uc_font_6x8);
//    uc1617s_draw_char(16, 18, 'c', GRAY_DARK, uc_font_6x8);

//    /* 整数 */
//    uc1617s_draw_int(0, 30, 12345, GRAY_BLACK, uc_font_8x16);
//    uc1617s_draw_int(0, 48, -6789, GRAY_BLACK, uc_font_8x16);
//    uc1617s_draw_int(0, 66, 0, GRAY_DARK, uc_font_8x16);

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[7/9] Char+Int", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);

    /* ==========================================================
     *  测试8: 抗锯齿
     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);

//    /* 普通字 vs 抗锯齿字 (并排对比) */
//    uc1617s_draw_string(2, 2, "Normal:", GRAY_BLACK, uc_font_5x7);
//    uc1617s_draw_string(2, 12, "AntiAlias:", GRAY_BLACK, uc_font_5x7);

//    uc1617s_draw_char(50, 2, 'H', GRAY_BLACK, uc_font_8x16);
//    uc1617s_draw_char(60, 2, 'i', GRAY_BLACK, uc_font_8x16);

//    uc1617s_draw_char_aa(50, 20, 'H', GRAY_BLACK, GRAY_WHITE, GRAY_DARK, uc_font_8x16);
//    uc1617s_draw_char_aa(60, 20, 'i', GRAY_BLACK, GRAY_WHITE, GRAY_DARK, uc_font_8x16);

//    /* 抗锯齿字符串 */
//    uc1617s_draw_string_aa(2, 42, "Hello World",
//                           GRAY_BLACK, GRAY_WHITE, GRAY_DARK, uc_font_8x16);

//    uc1617s_draw_string_aa(2, 62, "25.6C",
//                           GRAY_BLACK, GRAY_WHITE, GRAY_LIGHT, uc_font_8x16);

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "[8/9] AA", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
//    delay_ms(4000);

    /* ==========================================================
     *  测试9: 全部混合实战
     * ========================================================== */
    uc1617s_batch_begin();

    uc1617s_clear(GRAY_WHITE);

    /* 标题: 中文 */
    uc1617s_fill_rect(0, 0, 128, 18, GRAY_DARK);
    uc1617s_draw_string_cn(4, 1, "液晶显示屏",
                              GRAY_WHITE, GRAY_DARK,
                              _cn_font, CN_FONT_COUNT, uc_font_8x16);

    /* 温度: 整数 */
    uc1617s_draw_string_cn(4, 22, "温度:",
                           GRAY_BLACK, GRAY_WHITE,
                           _cn_font, CN_FONT_COUNT, uc_font_8x16);
    uc1617s_draw_int(44, 22, 256, GRAY_BLACK, uc_font_8x16);
    uc1617s_draw_string_cn(80, 22, "晶",
                           GRAY_BLACK, GRAY_WHITE,
                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

    /* 状态: 中英文混合 */
    uc1617s_draw_string_cn(4, 40, "屏幕OK",
                           GRAY_DARK, GRAY_WHITE,
                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

    /* 普通文字 */
    uc1617s_draw_string(4, 58, "ASCII test", GRAY_BLACK, uc_font_6x8);

    /* 灰度对比 */
    uc1617s_fill_rect(4, 70, 20, 10, GRAY_WHITE);
    uc1617s_fill_rect(28, 70, 20, 10, GRAY_LIGHT);
    uc1617s_fill_rect(52, 70, 20, 10, GRAY_DARK);
    uc1617s_fill_rect(76, 70, 20, 10, GRAY_BLACK);

    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
    uc1617s_draw_string(2, 89, "[9/9] Full", GRAY_LIGHT, uc_font_5x7);

    uc1617s_batch_end();
    delay_ms(4000);
//    /* ==========================================================
//     *  完成
//     * ========================================================== */
//    uc1617s_batch_begin();

//    uc1617s_clear(GRAY_WHITE);
//    uc1617s_fill_rect(10, 15, 108, 60, GRAY_LIGHT);
//    uc1617s_draw_rect(8, 13, 112, 64, GRAY_BLACK);

//    uc1617s_draw_string_cn(16, 18, "全部通过",
//                           GRAY_BLACK, GRAY_LIGHT,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(16, 38, "这是测试",
//                           GRAY_DARK, GRAY_LIGHT,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_draw_string_cn(16, 56, "显示OK",
//                           GRAY_BLACK, GRAY_LIGHT,
//                           _cn_font, CN_FONT_COUNT, uc_font_8x16);

//    uc1617s_fill_rect(0, 88, 128, 8, GRAY_DARK);
//    uc1617s_draw_string(2, 89, "Done!", GRAY_LIGHT, uc_font_5x7);

//    uc1617s_batch_end();
}



