#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification.h>
#include <bt/bt_service/bt.h>
#include <bt/bt_service/bt_hid_profile.h>

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    NotificationApp* notifications;
    Bt* bt;
    bool connected;
    bool vibration;
} MediaScrollerApp;

static void vibrate(MediaScrollerApp* app) {
    if(app->vibration) {
        notification_message(app->notifications, &sequence_single_vibro);
    }
}

static void render_callback(Canvas* canvas, void* ctx) {
    MediaScrollerApp* app = ctx;
    canvas_clear(canvas);
    
    // Title
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 5, AlignCenter, AlignTop, "MEDIA SCROLLER");
    
    // Divider line
    canvas_draw_line(canvas, 0, 15, 128, 15);
    
    // Connection status
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 5, 28, app->connected ? "STATUS: CONNECTED" : "STATUS: DISCONNECTED");
    
    // Controls info
    canvas_draw_str(canvas, 5, 40, "OK: Play/Pause");
    canvas_draw_str(canvas, 5, 52, "UP: Next Video");
    canvas_draw_str(canvas, 5, 64, "DOWN: Previous");
    
    // Footer
    canvas_draw_line(canvas, 0, 53, 128, 53);
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str_aligned(canvas, 64, 60, AlignCenter, AlignTop, 
        app->connected ? "READY TO USE!" : "ENABLE BT HID FIRST");
}

static void input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    MediaScrollerApp* app = ctx;
    
    if(input_event->type == InputTypeShort) {
        if(input_event->key == InputKeyBack) {
            furi_message_queue_put(app->event_queue, input_event, FuriWaitForever);
            return;
        }
        
        if(app->connected) {
            switch(input_event->key) {
            case InputKeyOk:
                bt_hid_media_key_press(app->bt, BtHidMediaKeyPlayPause);
                vibrate(app);
                break;
            case InputKeyUp:
                bt_hid_media_key_press(app->bt, BtHidMediaKeyNext);
                vibrate(app);
                break;
            case InputKeyDown:
                bt_hid_media_key_press(app->bt, BtHidMediaKeyPrev);
                vibrate(app);
                break;
            case InputKeyLeft: // Toggle vibration
                app->vibration = !app->vibration;
                if(app->vibration) vibrate(app);
                break;
            default:
                break;
            }
        }
    }
    view_port_update(app->view_port);
}

static MediaScrollerApp* app_alloc() {
    MediaScrollerApp* app = malloc(sizeof(MediaScrollerApp));
    
    // GUI setup
    app->view_port = view_port_alloc();
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    
    // Callbacks
    view_port_draw_callback_set(app->view_port, render_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    
    // Services
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->bt = furi_record_open(RECORD_BT);
    app->connected = bt_is_connected(app->bt);
    app->vibration = true; // Vibration enabled by default
    
    return app;
}

static void app_free(MediaScrollerApp* app) {
    furi_assert(app);
    
    // Free GUI
    view_port_enabled_set(app->view_port, false);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    
    // Close records
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_BT);
    
    free(app);
}

int32_t media_scroller_app(void* p) {
    UNUSED(p);
    MediaScrollerApp* app = app_alloc();
    
    InputEvent event;
    bool running = true;
    
    // Main loop
    while(running) {
        if(furi_message_queue_get(app->event_queue, &event, 100) == FuriStatusOk) {
            if(event.key == InputKeyBack && event.type == InputTypeShort) {
                running = false;
            }
        }
        
        // Update connection status
        bool new_state = bt_is_connected(app->bt);
        if(new_state != app->connected) {
            app->connected = new_state;
            view_port_update(app->view_port);
        }
    }
    
    app_free(app);
    return 0;
}
