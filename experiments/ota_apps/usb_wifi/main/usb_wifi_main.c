/*
 * usb_wifi — USB NCM network adapter bridging ESP32-S3 WiFi to PC over USB.
 *
 * UI flow: Scan → SSID list → Password keyboard → Save → Connect → Bridge
 */

#include "mia_host_abi.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_private/wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_net.h"

static const char *TAG = "usb_wifi";

#define MAX_NETWORKS 16
#define PASS_MAXLEN 63
#define LIST_VISIBLE 8

/* ── Keyboard layout ─────────────────────────────────────────────── */
static const char *KB_ROWS[] = {
    "abcdefghij",
    "klmnopqrst",
    "uvwxyz0123",
    "456789.:-_",
};
#define KB_ROW_COUNT 4
#define KB_ROW_LEN 10
/* Special action cells on a 5th row */
#define KB_SPECIAL_COUNT 3
#define KB_DONE 0
#define KB_DEL 1
#define KB_CANCEL 2
#define KB_TOTAL_ROWS (KB_ROW_COUNT + 1)

/* ── App state ───────────────────────────────────────────────────── */
typedef enum {
    STATE_SCAN,
    STATE_LIST,
    STATE_KEYBOARD,
    STATE_CONNECTING,
    STATE_RUNNING,
} AppState;

typedef struct {
    AppState state;
    /* WiFi scan results */
    MiaHostWifiNetwork networks[MAX_NETWORKS];
    int32_t network_count;
    int32_t list_sel;       /* selected network in list */
    int32_t list_scroll;    /* scroll offset for list */
    /* Keyboard */
    char password[PASS_MAXLEN + 1];
    int32_t pass_len;
    int32_t kb_row;
    int32_t kb_col;
    /* Selected SSID */
    char ssid[33];
    /* Bridge runtime */
    bool usb_ready;
    bool wifi_started;
    volatile bool wifi_connected;
    char usb_status[40];
    char wifi_status[48];
} UsbWifiState;

static UsbWifiState g;
static MiaHostWifiNetwork g_scan_results[MAX_NETWORKS];

/* ── Drawing helpers ─────────────────────────────────────────────── */

static void draw_title(const char *title) {
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 16, MIA_HOST_CYAN);
    /* truncate title to fit */
    char buf[54];
    snprintf(buf, sizeof(buf), "%.*s", 52, title);
    mia_host_draw_text(4, 4, buf, MIA_HOST_BLACK, MIA_HOST_CYAN);
}

static bool exit_pressed(void) {
    return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
           mia_host_button_down(MIA_HOST_BUTTON_START);
}

/* ── Credential file I/O ─────────────────────────────────────────── */

static bool load_credentials(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz) {
    ssid[0] = '\0';
    pass[0] = '\0';
    FILE *f = fopen("/sd/wifi.txt", "r");
    if (!f) return false;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *val = eq + 1;
        size_t vlen = strlen(val);
        while (vlen > 0 && (val[vlen-1] == '\n' || val[vlen-1] == '\r'))
            val[--vlen] = '\0';

        if (strcmp(line, "ssid") == 0 && vlen > 0) {
            snprintf(ssid, ssid_sz, "%.*s", (int)(ssid_sz - 1), val);
        } else if (strcmp(line, "password") == 0) {
            snprintf(pass, pass_sz, "%.*s", (int)(pass_sz - 1), val);
        }
    }
    fclose(f);
    return ssid[0] != '\0';
}

static void save_credentials(const char *ssid, const char *password) {
    FILE *f = fopen("/sd/wifi.txt", "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to write /sd/wifi.txt");
        return;
    }
    fprintf(f, "ssid=%s\n", ssid);
    fprintf(f, "password=%s\n", password);
    fclose(f);
    ESP_LOGI(TAG, "Saved credentials ssid='%s'", ssid);
}

/* ── USB NCM + WiFi bridge (from official ESP-IDF tusb_ncm example) ── */

static esp_err_t usb_recv_callback(void *buffer, uint16_t len, void *ctx) {
    (void)ctx;
    if (g.wifi_connected) {
        return esp_wifi_internal_tx(ESP_IF_WIFI_STA, buffer, len);
    }
    return ESP_OK;
}

static void wifi_pkt_free(void *eb, void *ctx) {
    (void)ctx;
    esp_wifi_internal_free_rx_buffer(eb);
}

static esp_err_t pkt_wifi2usb(void *buffer, uint16_t len, void *eb) {
    if (tinyusb_net_send_sync(buffer, len, eb, portMAX_DELAY) != ESP_OK) {
        esp_wifi_internal_free_rx_buffer(eb);
    }
    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data) {
    (void)arg; (void)data;
    if (base != WIFI_EVENT) return;

    if (id == WIFI_EVENT_STA_DISCONNECTED) {
        g.wifi_connected = false;
        snprintf(g.wifi_status, sizeof(g.wifi_status), "Disconnected, retry");
        esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, NULL);
        esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_CONNECTED) {
        g.wifi_connected = true;
        snprintf(g.wifi_status, sizeof(g.wifi_status), "Connected");
        esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, pkt_wifi2usb);
    }
}

static esp_err_t start_usb_ncm(void) {
    ESP_LOGI(TAG, "USB NCM init");
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_RETURN_ON_ERROR(err, TAG, "tinyusb_driver_install");
    }

    tinyusb_net_config_t net_config = {
        .on_recv_callback = usb_recv_callback,
        .free_tx_buffer = wifi_pkt_free,
        .user_context = NULL,
    };
    esp_read_mac(net_config.mac_addr, ESP_MAC_WIFI_STA);
    ESP_RETURN_ON_ERROR(tinyusb_net_init(&net_config), TAG, "tinyusb_net_init");

    g.usb_ready = true;
    snprintf(g.usb_status, sizeof(g.usb_status), "NCM ready");
    return ESP_OK;
}

static esp_err_t start_wifi_sta(const char *ssid, const char *password) {
    /* Init event loop (tolerate already-initialized) */
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL),
        TAG, "event register");

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "STA mode");

    wifi_config_t wc = {0};
    strncpy((char *)wc.sta.ssid, ssid, sizeof(wc.sta.ssid) - 1);
    strncpy((char *)wc.sta.password, password, sizeof(wc.sta.password) - 1);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wc), TAG, "wifi config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start");

    g.wifi_started = true;
    snprintf(g.wifi_status, sizeof(g.wifi_status), "Connecting...");
    return esp_wifi_connect();
}

static void stop_wifi(void) {
    if (!g.wifi_started) return;
    esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, NULL);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    g.wifi_connected = false;
    g.wifi_started = false;
}

/* ── State: SCAN ─────────────────────────────────────────────────── */

static void draw_scan(void) {
    mia_host_clear(MIA_HOST_BLACK);
    draw_title("USB WiFi");
    mia_host_draw_text(100, 110, "Scanning WiFi...", MIA_HOST_YELLOW, MIA_HOST_BLACK);
    mia_host_present();
}

static void do_scan(void) {
    g.state = STATE_SCAN;
    draw_scan();
    mia_host_wifi_off(); /* ensure clean state */
    mia_host_delay_ms(200);
    g.network_count = mia_host_wifi_scan(g_scan_results, MAX_NETWORKS);
    if (g.network_count < 0) g.network_count = 0;
    g.list_sel = 0;
    g.list_scroll = 0;
    g.state = STATE_LIST;
}

/* ── State: LIST ─────────────────────────────────────────────────── */

static void draw_list(void) {
    mia_host_clear(MIA_HOST_BLACK);
    draw_title("Select WiFi");

    if (g.network_count == 0) {
        mia_host_draw_text(100, 110, "No networks found", MIA_HOST_RED, MIA_HOST_BLACK);
        mia_host_draw_text(80, 222, "A:Rescan  SEL+ST:Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
        mia_host_present();
        return;
    }

    int32_t vis_end = g.list_scroll + LIST_VISIBLE;
    if (vis_end > g.network_count) vis_end = g.network_count;

    for (int32_t i = g.list_scroll; i < vis_end; ++i) {
        int32_t row = i - g.list_scroll;
        int32_t y = 26 + row * 14;
        bool sel = (i == g.list_sel);
        uint8_t bg = sel ? MIA_HOST_BLUE : MIA_HOST_BLACK;
        uint8_t fg = sel ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
        mia_host_fill_rect(0, y - 1, mia_host_screen_width(), 13, bg);
        char line[48];
        snprintf(line, sizeof(line), "%3ld %.*s",
                 (long)g_scan_results[i].rssi, 36, g_scan_results[i].ssid);
        mia_host_draw_text(4, y, line, fg, bg);
    }

    char bot[48];
    snprintf(bot, sizeof(bot), "%ld found  A:Select B:Rescan", (long)g.network_count);
    mia_host_draw_text(4, 224, bot, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

static void tick_list(void) {
    draw_list();

    /* wait for button release then press */
    mia_host_buttons_poll();
    /* Debounce: wait until no button pressed */
    while (mia_host_button_pressed(MIA_HOST_BUTTON_UP) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_A) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
        mia_host_buttons_poll();
        mia_host_delay_ms(10);
    }

    while (g.state == STATE_LIST) {
        mia_host_buttons_poll();
        if (exit_pressed()) {
            g.state = STATE_RUNNING; /* signal exit */
            return;
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && g.list_sel > 0) {
            --g.list_sel;
            if (g.list_sel < g.list_scroll) g.list_scroll = g.list_sel;
            draw_list();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) &&
            g.list_sel < g.network_count - 1) {
            ++g.list_sel;
            if (g.list_sel >= g.list_scroll + LIST_VISIBLE)
                g.list_scroll = g.list_sel - LIST_VISIBLE + 1;
            draw_list();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_A) && g.network_count > 0) {
            snprintf(g.ssid, sizeof(g.ssid), "%s", g_scan_results[g.list_sel].ssid);
            g.pass_len = 0;
            g.password[0] = '\0';
            g.kb_row = 0;
            g.kb_col = 0;
            g.state = STATE_KEYBOARD;
            return;
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
            do_scan();  /* rescan */
            return;
        }
        mia_host_delay_ms(20);
    }
}

/* ── State: KEYBOARD ─────────────────────────────────────────────── */

static void draw_keyboard(void) {
    mia_host_clear(MIA_HOST_BLACK);

    /* Title: SSID */
    char title[40];
    snprintf(title, sizeof(title), "PSK: %.*s", 38, g.ssid);
    draw_title(title);

    /* Password field (may scroll if too long) */
    int32_t show_start = 0;
    if (g.pass_len > 44) show_start = g.pass_len - 44;
    mia_host_fill_rect(0, 20, mia_host_screen_width(), 14, MIA_HOST_BLACK);
    mia_host_draw_text(4, 22, g.password + show_start, MIA_HOST_GREEN, MIA_HOST_BLACK);

    /* Keyboard grid: 5 rows, 10 cols, each cell 30px wide */
    int32_t cell_w = 30;
    int32_t cell_h = 16;
    int32_t kb_y0 = 40;

    for (int32_t r = 0; r < KB_TOTAL_ROWS; ++r) {
        int32_t y = kb_y0 + r * (cell_h + 2);
        int32_t cols = (r < KB_ROW_COUNT) ? KB_ROW_LEN : KB_SPECIAL_COUNT;

        for (int32_t c = 0; c < cols; ++c) {
            int32_t x = c * (cell_w + 2) + 4;
            bool sel = (r == g.kb_row && c == g.kb_col);
            uint8_t bg = sel ? MIA_HOST_BLUE : MIA_HOST_GRAY;
            uint8_t fg = sel ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
            mia_host_fill_rect(x, y, cell_w, cell_h, bg);

            char label[8];
            if (r < KB_ROW_COUNT) {
                label[0] = KB_ROWS[r][c];
                label[1] = '\0';
            } else {
                if (c == KB_DONE)      strcpy(label, "done");
                else if (c == KB_DEL)  strcpy(label, "del");
                else                   strcpy(label, "cancel");
            }
            mia_host_draw_text(x + 8, y + 4, label, fg, bg);
        }
    }

    mia_host_draw_text(4, 224, "A:Type B:Del SEL+ST:Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

static int32_t kb_cols_in_row(int32_t row) {
    return (row < KB_ROW_COUNT) ? KB_ROW_LEN : KB_SPECIAL_COUNT;
}

static void tick_keyboard(void) {
    draw_keyboard();

    /* release debounce */
    while (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_UP) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_A) ||
           mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
        mia_host_buttons_poll();
        mia_host_delay_ms(10);
    }

    while (g.state == STATE_KEYBOARD) {
        mia_host_buttons_poll();
        if (exit_pressed()) {
            g.state = STATE_RUNNING; /* signal exit */
            return;
        }

        if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT)) {
            --g.kb_col;
            if (g.kb_col < 0) {
                g.kb_row--;
                if (g.kb_row < 0) g.kb_row = KB_TOTAL_ROWS - 1;
                g.kb_col = kb_cols_in_row(g.kb_row) - 1;
            }
            draw_keyboard();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT)) {
            ++g.kb_col;
            if (g.kb_col >= kb_cols_in_row(g.kb_row)) {
                g.kb_col = 0;
                g.kb_row++;
                if (g.kb_row >= KB_TOTAL_ROWS) g.kb_row = 0;
            }
            draw_keyboard();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP)) {
            --g.kb_row;
            if (g.kb_row < 0) g.kb_row = KB_TOTAL_ROWS - 1;
            if (g.kb_col >= kb_cols_in_row(g.kb_row))
                g.kb_col = kb_cols_in_row(g.kb_row) - 1;
            draw_keyboard();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) {
            ++g.kb_row;
            if (g.kb_row >= KB_TOTAL_ROWS) g.kb_row = 0;
            if (g.kb_col >= kb_cols_in_row(g.kb_row))
                g.kb_col = kb_cols_in_row(g.kb_row) - 1;
            draw_keyboard();
        }

        if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
            if (g.kb_row < KB_ROW_COUNT) {
                /* type char */
                if (g.pass_len < PASS_MAXLEN) {
                    g.password[g.pass_len++] = KB_ROWS[g.kb_row][g.kb_col];
                    g.password[g.pass_len] = '\0';
                    draw_keyboard();
                }
            } else {
                if (g.kb_col == KB_DONE) {
                    /* save + proceed */
                    save_credentials(g.ssid, g.password);
                    g.state = STATE_CONNECTING;
                    return;
                } else if (g.kb_col == KB_DEL) {
                    if (g.pass_len > 0) {
                        g.password[--g.pass_len] = '\0';
                        draw_keyboard();
                    }
                } else {
                    /* cancel */
                    g.state = STATE_LIST;
                    return;
                }
            }
        }

        if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
            if (g.pass_len > 0) {
                g.password[--g.pass_len] = '\0';
                draw_keyboard();
            }
        }

        mia_host_delay_ms(20);
    }
}

/* ── State: CONNECTING ───────────────────────────────────────────── */

static void draw_connecting(void) {
    mia_host_clear(MIA_HOST_BLACK);
    draw_title("Connecting...");

    char line[64];
    snprintf(line, sizeof(line), "SSID: %s", g.ssid);
    mia_host_draw_text(4, 40, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    /* show password masked */
    snprintf(line, sizeof(line), "PSK:  %.*s", 40, g.password);
    mia_host_draw_text(4, 56, line, MIA_HOST_GRAY, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), "USB:  %s", g.usb_status);
    mia_host_draw_text(4, 80, line,
                       g.usb_ready ? MIA_HOST_GREEN : MIA_HOST_RED, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), "WiFi: %s", g.wifi_status);
    mia_host_draw_text(4, 96, line, MIA_HOST_YELLOW, MIA_HOST_BLACK);

    mia_host_draw_text(4, 130, "Connecting to WiFi...", MIA_HOST_CYAN, MIA_HOST_BLACK);
    mia_host_present();
}

static void tick_connecting(void) {
    /* Init USB NCM first, then WiFi */
    esp_err_t err = start_usb_ncm();
    if (err != ESP_OK) {
        snprintf(g.usb_status, sizeof(g.usb_status), "USB Error %d", (int)err);
    }

    err = start_wifi_sta(g.ssid, g.password);
    if (err != ESP_OK) {
        snprintf(g.wifi_status, sizeof(g.wifi_status), "Error %d", (int)err);
    }

    uint32_t start_ms = mia_host_millis();
    g.state = STATE_CONNECTING;
    draw_connecting();

    /* Wait up to 15s for WiFi connection */
    while (g.state == STATE_CONNECTING) {
        mia_host_buttons_poll();
        if (exit_pressed()) {
            g.state = STATE_RUNNING; /* signal exit */
            return;
        }

        uint32_t elapsed = mia_host_millis() - start_ms;
        if (g.wifi_connected) {
            g.state = STATE_RUNNING;
            return;
        }

        if (elapsed > 15000) {
            /* timeout */
            mia_host_clear(MIA_HOST_BLACK);
            draw_title("Connect Failed");
            mia_host_draw_text(4, 100, "WiFi connection timeout", MIA_HOST_RED, MIA_HOST_BLACK);
            mia_host_draw_text(4, 120, "A:Retry  B:Back to list", MIA_HOST_YELLOW, MIA_HOST_BLACK);
            mia_host_present();

            /* wait for input */
            while (1) {
                mia_host_buttons_poll();
                if (exit_pressed()) {
                    g.state = STATE_RUNNING;
                    return;
                }
                if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
                    /* retry: reconnect with same creds */
                    if (g.wifi_started) {
                        stop_wifi();
                    }
                    start_ms = mia_host_millis();
                    start_wifi_sta(g.ssid, g.password);
                    draw_connecting();
                    break;
                }
                if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
                    /* back to list */
                    if (g.wifi_started) stop_wifi();
                    g.state = STATE_LIST;
                    return;
                }
                mia_host_delay_ms(20);
            }
        }

        /* redraw every 500ms to show updated status */
        if (elapsed % 500 < 20) {
            draw_connecting();
        }
        mia_host_delay_ms(20);
    }
}

/* ── State: RUNNING ──────────────────────────────────────────────── */

static void draw_running(void) {
    char line[64];
    mia_host_clear(MIA_HOST_BLACK);
    draw_title("USB WiFi Bridge");

    snprintf(line, sizeof(line), "USB:  %s", g.usb_status);
    mia_host_draw_text(4, 36, line,
                       g.usb_ready ? MIA_HOST_GREEN : MIA_HOST_RED, MIA_HOST_BLACK);

    uint8_t wifi_color = g.wifi_connected ? MIA_HOST_GREEN : MIA_HOST_YELLOW;
    snprintf(line, sizeof(line), "WiFi: %s", g.wifi_status);
    mia_host_draw_text(4, 52, line, wifi_color, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), "SSID: %s", g.ssid);
    mia_host_draw_text(4, 68, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    mia_host_draw_text(4, 100, "USB NCM network adapter", MIA_HOST_CYAN, MIA_HOST_BLACK);
    mia_host_draw_text(4, 116, "Check PC for new NIC.", MIA_HOST_GRAY, MIA_HOST_BLACK);

    mia_host_draw_text(4, 224, "SEL+ST:Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

/* ── Main entry ──────────────────────────────────────────────────── */

int usb_wifi_main_impl(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (mia_host_abi_version() != 2) {
        mia_host_log("usb_wifi: ABI mismatch");
        return 1;
    }

    memset(&g, 0, sizeof(g));
    g.state = STATE_SCAN;

    /* Quick-start: if credentials already saved, ask user */
    char saved_ssid[33];
    char saved_pass[PASS_MAXLEN + 1];
    bool have_creds = load_credentials(saved_ssid, sizeof(saved_ssid),
                                       saved_pass, sizeof(saved_pass));

    if (have_creds) {
        mia_host_clear(MIA_HOST_BLACK);
        draw_title("USB WiFi");
        char line[48];
        snprintf(line, sizeof(line), "Saved: %.*s", 42, saved_ssid);
        mia_host_draw_text(4, 80, line, MIA_HOST_WHITE, MIA_HOST_BLACK);
        mia_host_draw_text(4, 110, "A:Connect  B:Rescan", MIA_HOST_YELLOW, MIA_HOST_BLACK);
        mia_host_draw_text(4, 224, "SEL+ST:Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
        mia_host_present();

        /* wait for input */
        bool decided = false;
        while (!decided) {
            mia_host_buttons_poll();
            if (exit_pressed()) {
                mia_host_clear(MIA_HOST_BLACK);
                mia_host_present();
                return 0;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
                snprintf(g.ssid, sizeof(g.ssid), "%s", saved_ssid);
                snprintf(g.password, sizeof(g.password), "%s", saved_pass);
                g.state = STATE_CONNECTING;
                decided = true;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
                decided = true;  /* fall through to scan */
            }
            mia_host_delay_ms(20);
        }
    }

    /* Main state machine loop */
    while (g.state != STATE_RUNNING || !g.wifi_started) {
        if (g.state == STATE_SCAN) {
            do_scan();
        } else if (g.state == STATE_LIST) {
            tick_list();
        } else if (g.state == STATE_KEYBOARD) {
            tick_keyboard();
        } else if (g.state == STATE_CONNECTING) {
            tick_connecting();
        } else if (g.state == STATE_RUNNING) {
            break;
        }

        /* Check if exit was requested via SEL+ST in a sub-state */
        if (g.state == STATE_RUNNING && !g.wifi_started) {
            /* exit signal from a sub-state before bridge started */
            mia_host_clear(MIA_HOST_BLACK);
            mia_host_present();
            return 0;
        }
    }

    /* RUNNING: display status, poll for exit */
    draw_running();
    uint32_t last_draw = 0;
    while (1) {
        mia_host_buttons_poll();
        if (exit_pressed()) break;

        uint32_t now = mia_host_millis();
        if (now - last_draw >= 1000) {
            last_draw = now;
            draw_running();
        }
        mia_host_delay_ms(20);
    }

    stop_wifi();
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_present();
    mia_host_log("usb_wifi: exit");
    return 0;
}
