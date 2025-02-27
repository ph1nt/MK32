#ifndef KEYMAP_C
#define KEYMAP_C

#include "keymap.h"

#include "key_definitions.h"
#include "keyboard_config.h"
#include "plugins.h"

// A bit different from QMK, default returns you to the first layer, LOWER and
// raise increase/lower layer by order.
#define DEFAULT 0x100
#define LOWER 0x101
#define RAISE 0x102
/** \brief Key Actions
 *
 * Mod bits:    43210
 *   bit 0      ||||+- Control
 *   bit 1      |||+-- Shift
 *   bit 2      ||+--- Alt
 *   bit 3      |+---- Gui
 *   bit 4      +----- LR flag(Left:0, Right:1)
 */
enum mods_bit {
    MOD_LCTL = 0x01,
    MOD_LSFT = 0x02,
    MOD_LALT = 0x04,
    MOD_LGUI = 0x08,
    MOD_RCTL = 0x11,
    MOD_RSFT = 0x12,
    MOD_RALT = 0x14,
    MOD_RGUI = 0x18,
};
// Keymaps are designed to be relatively interchangeable with QMK
enum custom_keycodes {
    QWERTY,
    NUM,
    PLUGINS,
};
// Set these for each layer and use when layers are needed in a hold-to use
// layer
enum layer_holds { QWERTY_H = LAYER_HOLD_BASE_VAL,
                   NUM_H,
                   FUNCS_H };

// array to hold names of layouts for oled
char default_layout_names[LAYERS][MAX_LAYOUT_NAME_LENGTH] = {
    "QWERTY",
    "RAISE",
    "LOWER",
};

/* select a keycode for your macro
 * important - first macro must be initialized as MACRO_BASE_VAL
 * */
#define MACROS_NUM 2
enum custom_macros {
    KC_CTRL_ALT_DELETE = MACRO_BASE_VAL,
    KC_ALT_F4,
};

/*define what the macros do
 * important- make sure you you put the macros in the same order as the their
 * enumeration
 */
uint16_t macros[MACROS_NUM][MACRO_LEN] = {
    // CTRL+ALT+DEL
    {KC_LCTRL, KC_LALT, KC_DEL},
    // ALT +F4
    {KC_RALT, KC_LALT, KC_NO}};

/* Encoder keys for each layer by order, and for each pad
 * First variable states what usage the encoder has
 */
uint16_t default_encoder_map[LAYERS][ENCODER_SIZE] = {
    // volume control
    {MEDIA_ENCODER, KC_AUDIO_VOL_UP, KC_AUDIO_VOL_DOWN, KC_AUDIO_MUTE},
    // mouse wheel
    {MOUSE_ENCODER, KC_MS_WH_UP, KC_MS_WH_DOWN, KC_NONE},
    // brightness control
    {KEY_ENCODER, KC_F14, KC_F15, KC_NONE}};
uint16_t default_slave_encoder_map[LAYERS][ENCODER_SIZE] = {
    // volume control
    {MEDIA_ENCODER, KC_AUDIO_VOL_UP, KC_AUDIO_VOL_DOWN, KC_AUDIO_MUTE},
    // mouse wheel
    {MOUSE_ENCODER, KC_MS_WH_UP, KC_MS_WH_DOWN, KC_NONE},
    // brightness control
    {KEY_ENCODER, KC_F14, KC_F15, KC_NONE}};

// TODO mod tap keys
#define LSFT(key) (key)
#define MT(key1, key2) (key2)
#define LT(key1, key2) (key2)
// Fillers to make layering more clear
#define _______ KC_TRNS
#define XXXXXXX KC_NO
// TODO mod tap keys
#define LSFT(key) (key)
#define MT(key1, key2) (key2)
#define LT(key1, key2) (key2)

#define KC_V_UP KC_AUDIO_VOL_UP
#define KC_V_DN KC_AUDIO_VOL_DOWN
#define KC_SLEP KC_SYSTEM_SLEEP
#define KC_PLUS LSFT(KC_MINUS)
#define KC_UNDS LSFT(KC_EQUAL)
#define KC_PIPE LSFT(KC_BSLS)
#define KC_EXLM LSFT(KC_1)
#define KC_AT LSFT(KC_2)
#define KC_HASH LSFT(KC_3)
#define KC_DLR LSFT(KC_4)
#define KC_PERC LSFT(KC_5)
#define KC_CIRC LSFT(KC_6)
#define KC_AMPR LSFT(KC_7)
#define KC_ASTR LSFT(KC_8)
#define KC_LPRN LSFT(KC_9)
#define KC_RPRN LSFT(KC_0)
#define KC_TILD KC_NONUS_HASH
#define LOW_TAB LT(LOWER, KC_TAB)
#define RSE_BSP LT(RAISE, KC_BSPC)
#define ENT_SFT MT(MOD_LSFT, KC_ENTER)
#define ESC_CMD MT(MOD_LGUI, KC_ESC)

// NOTE: For this keymap due to wiring constraints the the two last rows on the left are wired unconventionally
//  Each keymap is represented by an array, with an array that points to all the keymaps  by order
// clang-format off
    uint16_t _QWERTY[MATRIX_ROWS][KEYMAP_COLS]={
        {KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P     },
        {KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN  },
        {KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH  },
        {NUM,     KC_LCTRL,KC_LALT, LOWER,   KC_NO,   KC_SPACE,KC_NO,   RAISE,   KC_ENTER,KC_LGUI  }
    };
    uint16_t _NUM[MATRIX_ROWS][KEYMAP_COLS]={
        { KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0    },
        { KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_LEFT, KC_DOWN, KC_UP,   KC_RIGHT,KC_ENTER},
        { KC_F11,  KC_F12,  KC_F13,  KC_F14,  KC_F15,  KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10  },
        { NUM,     KC_LCTRL,KC_LALT, LOWER,   KC_NO,   KC_BSLS, KC_NO,   RAISE,   KC_ENTER,KC_LGUI }
    };
    uint16_t _PLUGINS[MATRIX_ROWS][KEYMAP_COLS]={
        {KC_GRV,  KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC },
        {KC_ESC,  KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC },
        {KC_TAB,  KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT },
        {NUM,     KC_LCTRL,KC_LALT, LOWER,   KC_NO,   KC_BSLS, KC_NO,   RAISE,   KC_ENTER,KC_LGUI }

    };
// clang-format on
// Create an array that points to the various keymaps
uint16_t (*default_layouts[])[MATRIX_ROWS][KEYMAP_COLS] = {&_QWERTY, &_NUM,
                                                           &_PLUGINS};

uint8_t current_layout = 0;

#endif
