#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/widget.h>
#include <gui/modules/submenu.h>

#include "libraries/mcp_can_2515.h"
#include "fsd_logic/fsd_handler.h"

#define TESLA_FSD_VERSION "1.0.0"

// Scene IDs
typedef enum {
    TeslaFSDSceneMainMenu,
    TeslaFSDSceneHWDetect,
    TeslaFSDSceneHWSelect,
    TeslaFSDSceneRunning,
    TeslaFSDSceneAbout,
    TeslaFSDSceneCount,
} TeslaFSDScene;

// View IDs
typedef enum {
    TeslaFSDViewSubmenu,
    TeslaFSDViewWidget,
} TeslaFSDView;

// Custom events
typedef enum {
    TeslaFSDEventHWDetected,
    TeslaFSDEventHWNotFound,
    TeslaFSDEventNoDevice,
    TeslaFSDEventSelectHW3,
    TeslaFSDEventSelectHW4,
} TeslaFSDEvent;

// Worker thread flags
typedef enum {
    WorkerFlagStop = (1 << 0),
} WorkerFlag;

typedef struct {
    // Flipper GUI
    Gui* gui;
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;
    Widget* widget;
    Submenu* submenu;

    // CAN hardware
    MCP2515* mcp_can;
    CANFRAME can_frame;

    // Worker thread
    FuriThread* worker_thread;
    FuriMutex* mutex;

    // FSD state (protected by mutex)
    TeslaHWVersion hw_version;
    FSDState fsd_state;
} TeslaFSDApp;

TeslaFSDApp* tesla_fsd_app_alloc(void);
void tesla_fsd_app_free(TeslaFSDApp* app);
int32_t tesla_fsd_main(void* p);
