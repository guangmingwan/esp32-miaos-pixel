/*
 * usb_wifi — USB NCM network adapter bridging ESP32-S3 WiFi to PC over USB.
 *
 * UI flow: Scan → SSID list → Password keyboard → Save → Connect → Bridge
 */

#include "mia_host_abi.h"
#include "usb_wifi_i18n.h"

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
#define MAX_SAVED 8

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
    uint32_t ip;
    uint32_t mask;
    uint32_t router;
    uint32_t dns;
    bool valid;
} DhcpLeaseInfo;

typedef struct {
    char ssid[33];
    char password[PASS_MAXLEN + 1];
} SavedCred;

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
    /* Saved credentials */
    SavedCred saved[MAX_SAVED];
    int32_t saved_count;
    /* Bridge runtime */
    bool usb_ready;
    bool wifi_started;
    volatile bool wifi_connected;
    char usb_status[40];
    char wifi_status[48];
    uint8_t mac_addr[6];
    DhcpLeaseInfo lease;
} UsbWifiState;

static UsbWifiState g;
static MiaHostWifiNetwork g_scan_results[MAX_NETWORKS];
static portMUX_TYPE g_lease_lock = portMUX_INITIALIZER_UNLOCKED;

/* ── Drawing helpers ─────────────────────────────────────────────── */

static void draw_title(const char *title) {
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 16, MIA_HOST_CYAN);
    /* truncate title to fit */
    char buf[54];
    snprintf(buf, sizeof(buf), "%.*s", 52, title);
    mia_host_draw_text(4, mia_host_text_y_centered(0, 16), buf, MIA_HOST_BLACK,
                       MIA_HOST_CYAN);
}

static bool exit_pressed(void) {
    return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
           mia_host_button_down(MIA_HOST_BUTTON_START);
}

static uint16_t read_be16(const uint8_t *data) {
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static uint32_t read_ipv4(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static void format_ipv4(uint32_t address, char output[16]) {
    if (address == 0) {
        snprintf(output, 16, "--");
        return;
    }
    snprintf(output, 16, "%lu.%lu.%lu.%lu",
             (unsigned long)((address >> 24) & 0xff),
             (unsigned long)((address >> 16) & 0xff),
             (unsigned long)((address >> 8) & 0xff),
             (unsigned long)(address & 0xff));
}

static void clear_dhcp_lease(void) {
    portENTER_CRITICAL(&g_lease_lock);
    memset(&g.lease, 0, sizeof(g.lease));
    portEXIT_CRITICAL(&g_lease_lock);
}

static DhcpLeaseInfo get_dhcp_lease(void) {
    DhcpLeaseInfo lease;
    portENTER_CRITICAL(&g_lease_lock);
    lease = g.lease;
    portEXIT_CRITICAL(&g_lease_lock);
    return lease;
}

static void inspect_dhcp_reply(const void *buffer, uint16_t len) {
    const uint8_t *frame = (const uint8_t *)buffer;
    if (len < 14) return;

    size_t l2_header_len = 14;
    uint16_t ether_type = read_be16(frame + 12);
    for (int tags = 0; tags < 2 && (ether_type == 0x8100 || ether_type == 0x88a8);
         ++tags) {
        if (len < l2_header_len + 4) return;
        ether_type = read_be16(frame + l2_header_len + 2);
        l2_header_len += 4;
    }
    if (ether_type != 0x0800 || len < l2_header_len + 20 + 8 + 240) return;

    const uint8_t *ip = frame + l2_header_len;
    size_t ip_header_len = (size_t)(ip[0] & 0x0f) * 4;
    if ((ip[0] >> 4) != 4 || ip_header_len < 20 ||
        len < l2_header_len + ip_header_len + 8 + 240 || ip[9] != 17 ||
        (read_be16(ip + 6) & 0x1fff) != 0) return;

    uint16_t ip_total_len = read_be16(ip + 2);
    const uint8_t *udp = ip + ip_header_len;
    uint16_t udp_len = read_be16(udp + 4);
    if (read_be16(udp) != 67 || read_be16(udp + 2) != 68 ||
        udp_len < 8 + 240 || ip_total_len < ip_header_len + udp_len ||
        len < l2_header_len + ip_header_len + udp_len) return;

    const uint8_t *dhcp = udp + 8;
    size_t dhcp_len = udp_len - 8;
    if (dhcp[0] != 2 || dhcp[236] != 0x63 || dhcp[237] != 0x82 ||
        dhcp[238] != 0x53 || dhcp[239] != 0x63) return;

    DhcpLeaseInfo lease = get_dhcp_lease();
    uint32_t reply_ip = read_ipv4(dhcp + 16);
    if (reply_ip == 0) reply_ip = read_ipv4(dhcp + 12);
    if (reply_ip == 0) {
        uint32_t destination = read_ipv4(ip + 16);
        if (destination != 0 && destination != 0xffffffff) reply_ip = destination;
    }
    if (reply_ip != 0) lease.ip = reply_ip;
    uint8_t message_type = 0;
    size_t offset = 240;
    while (offset < dhcp_len) {
        uint8_t option = dhcp[offset++];
        if (option == 0) continue;
        if (option == 255) break;
        if (offset >= dhcp_len) return;
        uint8_t option_len = dhcp[offset++];
        if (option_len > dhcp_len - offset) return;

        if (option == 1 && option_len >= 4) {
            lease.mask = read_ipv4(dhcp + offset);
        } else if (option == 3 && option_len >= 4) {
            lease.router = read_ipv4(dhcp + offset);
        } else if (option == 6 && option_len >= 4) {
            lease.dns = read_ipv4(dhcp + offset);
        } else if (option == 53 && option_len == 1) {
            message_type = dhcp[offset];
        }
        offset += option_len;
    }

    if ((message_type != 2 && message_type != 5) || lease.ip == 0) return;
    lease.valid = true;
    portENTER_CRITICAL(&g_lease_lock);
    g.lease = lease;
    portEXIT_CRITICAL(&g_lease_lock);

    char ip_text[16];
    format_ipv4(lease.ip, ip_text);
    ESP_LOGI(TAG, "DHCP %s for USB host: %s",
             message_type == 5 ? "ACK" : "OFFER", ip_text);
}

/* ── Credential file I/O ─────────────────────────────────────────── */

static const char *find_saved_password(const char *ssid) {
    for (int32_t i = 0; i < g.saved_count; ++i) {
        if (strcmp(g.saved[i].ssid, ssid) == 0) {
            return g.saved[i].password;
        }
    }
    return NULL;
}

static void load_all_credentials(void) {
    g.saved_count = 0;
    FILE *f = fopen("/sd/wifi.txt", "r");
    if (!f) return;

    char line[128];
    SavedCred pending = {0};
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        size_t vlen = strlen(val);
        while (vlen > 0 && (val[vlen - 1] == '\n' || val[vlen - 1] == '\r'))
            val[--vlen] = '\0';

        if (strcmp(key, "ssid") == 0) {
            if (pending.ssid[0] != '\0' && g.saved_count < MAX_SAVED) {
                g.saved[g.saved_count++] = pending;
            }
            memset(&pending, 0, sizeof(pending));
            snprintf(pending.ssid, sizeof(pending.ssid), "%s", val);
        } else if (strcmp(key, "password") == 0 && pending.ssid[0] != '\0') {
            snprintf(pending.password, sizeof(pending.password), "%s", val);
        }
    }
    if (pending.ssid[0] != '\0' && g.saved_count < MAX_SAVED) {
        g.saved[g.saved_count++] = pending;
    }
    fclose(f);
}

static void save_all_credentials(void) {
    FILE *f = fopen("/sd/wifi.txt", "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to write /sd/wifi.txt");
        return;
    }
    for (int32_t i = 0; i < g.saved_count; ++i) {
        fprintf(f, "ssid=%s\n", g.saved[i].ssid);
        fprintf(f, "password=%s\n", g.saved[i].password);
    }
    fclose(f);
    ESP_LOGI(TAG, "Saved %ld credentials", (long)g.saved_count);
}

static void save_credential(const char *ssid, const char *password) {
    int32_t existing = -1;
    for (int32_t i = 0; i < g.saved_count; ++i) {
        if (strcmp(g.saved[i].ssid, ssid) == 0) {
            existing = i;
            break;
        }
    }

    int32_t end = existing >= 0 ? existing : g.saved_count;
    if (end >= MAX_SAVED) end = MAX_SAVED - 1;
    for (int32_t i = end; i > 0; --i) {
        g.saved[i] = g.saved[i - 1];
    }
    snprintf(g.saved[0].ssid, sizeof(g.saved[0].ssid), "%s", ssid);
    snprintf(g.saved[0].password, sizeof(g.saved[0].password), "%s", password);
    if (existing < 0 && g.saved_count < MAX_SAVED) ++g.saved_count;
    save_all_credentials();
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
    inspect_dhcp_reply(buffer, len);
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
        clear_dhcp_lease();
        snprintf(g.wifi_status, sizeof(g.wifi_status), "%s", usb_wifi_text()->disconnected);
        esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, NULL);
        esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_CONNECTED) {
        g.wifi_connected = true;
        snprintf(g.wifi_status, sizeof(g.wifi_status), "%s", usb_wifi_text()->connected);
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
    memcpy(g.mac_addr, net_config.mac_addr, sizeof(g.mac_addr));
    ESP_RETURN_ON_ERROR(tinyusb_net_init(&net_config), TAG, "tinyusb_net_init");

    g.usb_ready = true;
    snprintf(g.usb_status, sizeof(g.usb_status), "%s", usb_wifi_text()->ncm_ready);
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
    snprintf(g.wifi_status, sizeof(g.wifi_status), "%s", usb_wifi_text()->connecting);
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
    clear_dhcp_lease();
}

/* ── State: SCAN ─────────────────────────────────────────────────── */

static void draw_scan(void) {
    mia_host_clear(MIA_HOST_BLACK);
    draw_title(usb_wifi_text()->title_usb_wifi);
    mia_host_draw_text(100, 110, usb_wifi_text()->scanning, MIA_HOST_YELLOW, MIA_HOST_BLACK);
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
    draw_title(usb_wifi_text()->select_wifi);

    if (g.network_count == 0) {
        mia_host_draw_text(100, 110, usb_wifi_text()->no_networks, MIA_HOST_RED, MIA_HOST_BLACK);
        mia_host_draw_text(80, 222, usb_wifi_text()->rescan_exit_hint,
                           MIA_HOST_GRAY, MIA_HOST_BLACK);
        mia_host_present();
        return;
    }

    int32_t vis_end = g.list_scroll + LIST_VISIBLE;
    if (vis_end > g.network_count) vis_end = g.network_count;

    for (int32_t i = g.list_scroll; i < vis_end; ++i) {
        int32_t row = i - g.list_scroll;
        int32_t y = 26 + row * 14;
        bool sel = (i == g.list_sel);
        bool saved = (find_saved_password(g_scan_results[i].ssid) != NULL);
        uint8_t bg = sel ? MIA_HOST_BLUE : MIA_HOST_BLACK;
        uint8_t fg = sel ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
        mia_host_fill_rect(0, y - 1, mia_host_screen_width(), 13, bg);
        char line[48];
        snprintf(line, sizeof(line), "%3ld %.*s%s",
                 (long)g_scan_results[i].rssi, 32, g_scan_results[i].ssid,
                 saved ? " *" : "");
        mia_host_draw_text(4, y, line, fg, bg);
    }

    char bot[48];
    snprintf(bot, sizeof(bot), usb_wifi_text()->list_hint, (long)g.network_count);
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
            const char *saved = find_saved_password(g.ssid);
            if (saved) {
                snprintf(g.password, sizeof(g.password), "%s", saved);
                g.pass_len = strlen(g.password);
            } else {
                g.pass_len = 0;
                g.password[0] = '\0';
            }
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
    const UsbWifiText *text = usb_wifi_text();
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

    /* Keep every row and every label centered for the active font. */
    const int32_t gap = 2;
    const int32_t regular_cell_w = 29;
    const int32_t cell_h = 16;
    const int32_t kb_y0 = 40;
    const int32_t text_h = mia_host_text_height();
    const int32_t grid_w = KB_ROW_LEN * regular_cell_w +
                           (KB_ROW_LEN - 1) * gap;

    for (int32_t r = 0; r < KB_TOTAL_ROWS; ++r) {
        int32_t y = kb_y0 + r * (cell_h + 2);
        int32_t cols = (r < KB_ROW_COUNT) ? KB_ROW_LEN : KB_SPECIAL_COUNT;
        int32_t cell_w = r < KB_ROW_COUNT
                             ? regular_cell_w
                             : (grid_w - (KB_SPECIAL_COUNT - 1) * gap) /
                                   KB_SPECIAL_COUNT;
        int32_t row_w = cols * cell_w + (cols - 1) * gap;
        int32_t row_x = (mia_host_screen_width() - row_w) / 2;

        for (int32_t c = 0; c < cols; ++c) {
            int32_t x = row_x + c * (cell_w + gap);
            bool sel = (r == g.kb_row && c == g.kb_col);
            uint8_t bg = sel ? MIA_HOST_BLUE : MIA_HOST_GRAY;
            uint8_t fg = MIA_HOST_BLACK;
            mia_host_fill_rect(x, y, cell_w, cell_h, bg);

            char label[8];
            if (r < KB_ROW_COUNT) {
                label[0] = KB_ROWS[r][c];
                label[1] = '\0';
            } else {
                if (c == KB_DONE)      snprintf(label, sizeof(label), "%s", text->key_done);
                else if (c == KB_DEL)  snprintf(label, sizeof(label), "%s", text->key_delete);
                else                   snprintf(label, sizeof(label), "%s", text->key_cancel);
            }
            int32_t tw = mia_host_text_width(label);
            int32_t tx = x + (cell_w - tw) / 2;
            int32_t ty = y + (cell_h - text_h) / 2;
            mia_host_draw_text(tx, ty, label, fg, bg);
        }
    }

    mia_host_draw_text(4, 224, text->kb_hint, MIA_HOST_GRAY, MIA_HOST_BLACK);
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
                    save_credential(g.ssid, g.password);
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
    draw_title(usb_wifi_text()->connecting);

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

    mia_host_draw_text(4, 130, usb_wifi_text()->connecting, MIA_HOST_CYAN, MIA_HOST_BLACK);
    mia_host_present();
}

static void tick_connecting(void) {
    /* Init USB NCM first, then WiFi */
    esp_err_t err = start_usb_ncm();
    if (err != ESP_OK) {
        snprintf(g.usb_status, sizeof(g.usb_status), usb_wifi_text()->usb_error_fmt,
                 (int)err);
    }

    err = start_wifi_sta(g.ssid, g.password);
    if (err != ESP_OK) {
        snprintf(g.wifi_status, sizeof(g.wifi_status), usb_wifi_text()->error_fmt,
                 (int)err);
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
            draw_title(usb_wifi_text()->connect_failed);
            mia_host_draw_text(4, 100, usb_wifi_text()->wifi_timeout,
                               MIA_HOST_RED, MIA_HOST_BLACK);
            mia_host_draw_text(4, 120, usb_wifi_text()->retry_back_hint,
                               MIA_HOST_YELLOW, MIA_HOST_BLACK);
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
    const UsbWifiText *text = usb_wifi_text();
    DhcpLeaseInfo lease = get_dhcp_lease();
    wifi_ap_record_t ap_info = {0};
    bool have_ap_info = esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
    char ip[16], router[16], mask[16], dns[16], mac[18];
    char line[64];
    format_ipv4(lease.valid ? lease.ip : 0, ip);
    format_ipv4(lease.valid ? lease.router : 0, router);
    format_ipv4(lease.valid ? lease.mask : 0, mask);
    format_ipv4(lease.valid ? lease.dns : 0, dns);
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             g.mac_addr[0], g.mac_addr[1], g.mac_addr[2],
             g.mac_addr[3], g.mac_addr[4], g.mac_addr[5]);

    mia_host_clear(MIA_HOST_BLACK);
    draw_title(text->bridge_title);

    snprintf(line, sizeof(line), "USB:  %s", g.usb_status);
    mia_host_draw_text(4, 20, line,
                       g.usb_ready ? MIA_HOST_GREEN : MIA_HOST_RED, MIA_HOST_BLACK);

    uint8_t wifi_color = g.wifi_connected ? MIA_HOST_GREEN : MIA_HOST_YELLOW;
    snprintf(line, sizeof(line), "WiFi: %s", g.wifi_status);
    mia_host_draw_text(4, 36, line, wifi_color, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), "SSID: %s", g.ssid);
    mia_host_draw_text(4, 52, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    if (have_ap_info) {
        snprintf(line, sizeof(line), text->signal_fmt, ap_info.rssi);
    } else {
        snprintf(line, sizeof(line), "%s", text->signal_unavailable);
    }
    uint8_t signal_color = !have_ap_info ? MIA_HOST_GRAY :
                           (ap_info.rssi >= -67 ? MIA_HOST_GREEN :
                            (ap_info.rssi >= -75 ? MIA_HOST_YELLOW : MIA_HOST_RED));
    mia_host_draw_text(4, 74, line, signal_color, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), text->ip_fmt, ip);
    mia_host_draw_text(4, 92, line, lease.valid ? MIA_HOST_WHITE : MIA_HOST_GRAY,
                       MIA_HOST_BLACK);
    snprintf(line, sizeof(line), text->gateway_fmt, router);
    mia_host_draw_text(4, 110, line, lease.router ? MIA_HOST_WHITE : MIA_HOST_GRAY,
                       MIA_HOST_BLACK);
    snprintf(line, sizeof(line), text->mask_fmt, mask);
    mia_host_draw_text(4, 128, line, lease.mask ? MIA_HOST_WHITE : MIA_HOST_GRAY,
                       MIA_HOST_BLACK);
    snprintf(line, sizeof(line), text->mac_fmt, mac);
    mia_host_draw_text(4, 146, line, MIA_HOST_WHITE, MIA_HOST_BLACK);
    mia_host_draw_text(4, 164, text->link_speed, MIA_HOST_CYAN, MIA_HOST_BLACK);
    snprintf(line, sizeof(line), text->dns_fmt, dns);
    mia_host_draw_text(4, 182, line, lease.dns ? MIA_HOST_WHITE : MIA_HOST_GRAY,
                       MIA_HOST_BLACK);

    mia_host_draw_text(4, 224, text->running_hint,
                       MIA_HOST_GRAY, MIA_HOST_BLACK);
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

    load_all_credentials();

    if (g.saved_count > 0) {
        SavedCred *recent = &g.saved[0];
        mia_host_clear(MIA_HOST_BLACK);
        draw_title(usb_wifi_text()->title_usb_wifi);
        char line[48];
        snprintf(line, sizeof(line), usb_wifi_text()->saved_fmt, 42, recent->ssid);
        mia_host_draw_text(4, 80, line, MIA_HOST_WHITE, MIA_HOST_BLACK);
        mia_host_draw_text(4, 110, usb_wifi_text()->saved_controls,
                           MIA_HOST_YELLOW, MIA_HOST_BLACK);
        mia_host_draw_text(4, 146, usb_wifi_text()->win10_tips, MIA_HOST_YELLOW, MIA_HOST_BLACK);
        mia_host_draw_text(4, 164, usb_wifi_text()->win10_step1, MIA_HOST_WHITE, MIA_HOST_BLACK);
        mia_host_draw_text(4, 182, usb_wifi_text()->win10_step2, MIA_HOST_WHITE, MIA_HOST_BLACK);
        mia_host_draw_text(4, 200, usb_wifi_text()->win10_step3, MIA_HOST_WHITE, MIA_HOST_BLACK);
        mia_host_draw_text(4, 224, usb_wifi_text()->exit_hint, MIA_HOST_GRAY, MIA_HOST_BLACK);
        mia_host_present();

        uint32_t saved_screen_ms = mia_host_millis();
        bool decided = false;
        while (!decided) {
            mia_host_buttons_poll();
            if (exit_pressed()) {
                mia_host_clear(MIA_HOST_BLACK);
                mia_host_present();
                return 0;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
                decided = true;  /* fall through to scan */
            } else if (mia_host_button_pressed(MIA_HOST_BUTTON_A) ||
                       mia_host_millis() - saved_screen_ms >= 2000) {
                snprintf(g.ssid, sizeof(g.ssid), "%s", recent->ssid);
                snprintf(g.password, sizeof(g.password), "%s", recent->password);
                g.state = STATE_CONNECTING;
                decided = true;
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
