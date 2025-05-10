#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification.h>
#include <bt/bt_service/bt.h>

typedef struct {
    bool connected;
} MediaScrollerApp;

static void draw_callback(Canvas* canvas, void* ctx) {
    MediaScrollerApp* app = ctx;
    canvas_clear(canvas);
    canvas_draw_str(canvas, 0, 10, "Media Scroller");
    canvas_draw_str(canvas, 0, 30, app->connected ? "Connected!" : "Disconnected");
    canvas_draw_str(canvas, 0, 50, "OK: Play/Pause");
}

static void input_callback(InputEvent* input, void* ctx) {
    MediaScrollerApp* app = ctx;
    if(input->key == InputKeyOk && app->connected) {
        bt_hid_media_key_press(furi_record_open(RECORD_BT), BtHidMediaKeyPlayPause);
    }
}

int32_t media_scroller_app() {
    MediaScrollerApp* app = malloc(sizeof(MediaScrollerApp));
    app->connected = bt_is_connected(furi_record_open(RECORD_BT));
    
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, app);
    view_port_input_callback_set(view_port, input_callback, app);
    
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    
    while(1) {
        app->connected = bt_is_connected(furi_record_open(RECORD_BT));
        view_port_update(view_port);
        furi_delay_ms(500);
    }
    
    return 0;
}
