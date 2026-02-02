/**
 * @file rfid_uart.c
 * @brief Flipper Zero Application: RFID to UART Bridge
 * * This application scans for 125kHz RFID tags (LFRFID) and transmits 
 * the detected Unique ID (UID) over the GPIO UART pins.
 * * Hardware Connections:
 * - Flipper Pin 13 (TX) -> Receiver RX
 * - Flipper Pin 14 (RX) -> Receiver TX
 * - Flipper GND       -> Receiver GND
 */
#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <lfrfid/lfrfid_worker.h>
#include <lfrfid/protocols/lfrfid_protocols.h>

// --- CONFIGURATION ---

// FuriHalSerialIdUsart maps to GPIO Pins 13 (TX) and 14 (RX) on Flipper Zero
#define UART_CH FuriHalSerialIdUsart
#define BAUDRATE 9600

// --- EVENT SYSTEM ---

/**
 * @brief Event types used to notify the main thread
 */
typedef enum {
    EventTypeTick,      // Timer tick (unused in this simple app)
    EventTypeKey,       // Hardware button inputs
    EventTypeRfidRead   // RFID tag successfully read
} EventType;

/**
 * @brief Event structure passed from callbacks to the main loop via Message Queue
 */
typedef struct {
    EventType type;
    InputEvent input;       // Payload for Key events
    char rfid_data[32];     // Payload for RFID events (Hex String of UID)
} AppEvent;

// --- APP STATE ---

/**
 * @brief Main application state structure
 */
typedef struct {
    LFRFIDWorker* lfrfid_worker;        // The background worker handling RFID hardware
    ProtocolDict* dict;                 // Dictionary to decode RFID protocols
    Gui* gui;                           // GUI instance
    ViewPort* view_port;                // Viewport for rendering
    FuriHalSerialHandle* serial_handle; // Handle for UART communication
    FuriMessageQueue* event_queue;      // Queue to pass events to main thread
    FuriString* temp_str;               // Helper string for UI rendering
} RfidUartApp;

// --- UART SENDER ---

/**
 * @brief Transmits a string followed by a newline over UART
 * @param app Pointer to app state
 * @param str Null-terminated string to send
 */
void send_uart_string(RfidUartApp* app, const char* str) {
    if(app->serial_handle) {
        furi_hal_serial_tx(app->serial_handle, (uint8_t*)str, strlen(str));
        furi_hal_serial_tx(app->serial_handle, (uint8_t*)"\r\n", 2); // Standard CRLF
    }
}

// --- RFID CALLBACK (Runs in Background Worker Thread) ---

/**
 * @brief Callback fired when the RFID worker detects a tag.
 * * @warning This runs in a separate thread/context. Do NOT perform blocking operations
 * (like heavy calculations or UI updates) here. Instead, package the data
 * and send it to the main thread via the Event Queue.
 */
static void lfrfid_read_callback(LFRFIDWorkerReadResult result, ProtocolId protocol, void* context) {
    RfidUartApp* app = context;

    if(result == LFRFIDWorkerReadDone) {
        // 1. Get the size of the detected tag data
        size_t data_size = protocol_dict_get_data_size(app->dict, protocol);
        
        // 2. Sanity check to prevent buffer overflow (most tags are 5-7 bytes)
        if(data_size > 0 && data_size < 10) { 
            uint8_t* data = malloc(data_size);
            protocol_dict_get_data(app->dict, protocol, data, data_size);

            // 3. Prepare the event
            AppEvent event;
            event.type = EventTypeRfidRead;
            event.rfid_data[0] = '\0'; // Initialize string
            
            // 4. Convert Raw Bytes -> Hex String
            // We do this manually here to avoid using furi_string (which might be thread-unsafe)
            // or complex libraries inside a callback.
            char* ptr = event.rfid_data;
            for(size_t i = 0; i < data_size; i++) {
                // formatting: %02X ensures 0x5 becomes "05"
                ptr += snprintf(ptr, 32 - (ptr - event.rfid_data), "%02X", data[i]);
            }
            free(data);

            // 5. Send to Main Thread
            // '0' timeout means if queue is full, drop the packet (don't block)
            furi_message_queue_put(app->event_queue, &event, 0);
        }
    }
}

// --- INPUT CALLBACK (Runs in Input Thread) ---

/**
 * @brief Handles button presses on the Flipper
 */
static void input_callback(InputEvent* input_event, void* ctx) {
    RfidUartApp* app = ctx;
    AppEvent event;
    event.type = EventTypeKey;
    event.input = *input_event;
    furi_message_queue_put(app->event_queue, &event, 0);
}

// --- GUI RENDER (Runs in GUI Thread) ---

/**
 * @brief Draws the screen contents
 */
static void render_callback(Canvas* canvas, void* ctx) {
    RfidUartApp* app = ctx;
    
    canvas_clear(canvas);
    
    // Header
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "RFID -> UART Bridge");
    
    // Instructions
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 24, "Scanning...");
    canvas_draw_str(canvas, 2, 36, "TX: Pin 13  RX: Pin 14");
    
    // Last Scanned ID
    if (furi_string_size(app->temp_str) > 0) {
        canvas_draw_str(canvas, 2, 50, "Sent:");
        canvas_draw_str(canvas, 30, 50, furi_string_get_cstr(app->temp_str));
    }
}

// --- MAIN ENTRY POINT ---

int32_t rfid_uart_app(void* p) {
    UNUSED(p);
    
    // 1. Allocation
    RfidUartApp* app = malloc(sizeof(RfidUartApp));
    app->temp_str = furi_string_alloc();
    // Queue holds up to 8 events
    app->event_queue = furi_message_queue_alloc(8, sizeof(AppEvent));

    // 2. Init UART
    // Acquire control of the Expansion Header Serial pins
    app->serial_handle = furi_hal_serial_control_acquire(UART_CH);
    if(app->serial_handle) {
        furi_hal_serial_init(app->serial_handle, BAUDRATE);
    }

    // 3. Init GUI
    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, render_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    // 4. Init RFID Worker
    // Allocate protocol dictionary (supports EM4100, HID, etc.)
    app->dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    app->lfrfid_worker = lfrfid_worker_alloc(app->dict);
    
    // Start the background thread.
    // NOTE: We start the thread ONCE here. We do not stop/start the *thread* repeatedly.
    lfrfid_worker_start_thread(app->lfrfid_worker);
    
    // Start the actual reading action
    lfrfid_worker_read_start(app->lfrfid_worker, LFRFIDWorkerReadTypeAuto, lfrfid_read_callback, app);

    // 5. Main Event Loop
    AppEvent event;
    bool running = true;
    
    while(running) {
        // Block until an event is received (Save battery)
        FuriStatus status = furi_message_queue_get(app->event_queue, &event, FuriWaitForever);
        
        if(status == FuriStatusOk) {
            
            // --- Handle Inputs (Back Button) ---
            if(event.type == EventTypeKey) {
                if(event.input.key == InputKeyBack && event.input.type == InputTypeShort) {
                    running = false;
                }
            }
            
            // --- Handle RFID Reads ---
            else if(event.type == EventTypeRfidRead) {
                // Update internal string for the UI
                furi_string_set(app->temp_str, event.rfid_data);
                view_port_update(app->view_port); // Trigger redraw

                // Send data over UART
                FURI_LOG_I("RFID", "Sent: %s", event.rfid_data);
                send_uart_string(app, event.rfid_data);
                
                // Haptic Feedback
                furi_hal_vibro_on(true);
                furi_delay_ms(100);
                furi_hal_vibro_on(false);

                // --- Debounce Logic ---
                // To prevent spamming the UART if the fob is held against the reader:
                
                // 1. Stop the *reading process* (keep thread alive)
                lfrfid_worker_stop(app->lfrfid_worker); 
                
                // 2. Wait 1 second
                furi_delay_ms(1000); 
                
                // 3. Resume the *reading process*
                lfrfid_worker_read_start(app->lfrfid_worker, LFRFIDWorkerReadTypeAuto, lfrfid_read_callback, app);
            }
        }
    }

    // 6. Cleanup 
    // CRITICAL: Order matters to prevent 'furi_check failed' crashes.

    // A. Stop the worker action
    lfrfid_worker_stop(app->lfrfid_worker);
    
    // B. Stop the thread. This must be done BEFORE freeing the worker.
    lfrfid_worker_stop_thread(app->lfrfid_worker);
    
    // C. Free worker memory
    lfrfid_worker_free(app->lfrfid_worker);
    protocol_dict_free(app->dict);

    // D. Cleanup GUI
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);

    // E. Cleanup UART
    if(app->serial_handle) {
        furi_hal_serial_deinit(app->serial_handle);
        furi_hal_serial_control_release(app->serial_handle);
    }

    // F. Free generic resources
    furi_message_queue_free(app->event_queue);
    furi_string_free(app->temp_str);
    free(app);
    
    return 0;
}