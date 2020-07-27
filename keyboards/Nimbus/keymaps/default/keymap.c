/*
Copyright 2020 Cloudboards

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H
#include <print.h>

/* HARDCODED PARAMETERS */
#define BOOTTIME 3000
#define ENCODER_HOLD_DURATION 750

/* VARIABLE PARAMETERS */
bool numlock_status = false;
bool encoder_button_status = true;
bool encoder_button_status_previous = true;
bool encoder_hold = false;
bool encoder_hold_previous = false;

int8_t encoder_mode = 0;
bool encoder_hue_sat = true; // bool for change hue/sat mode of encoder

bool boot_complete = false;
static uint16_t start_time;
uint16_t encoder_start_time;


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

	LAYOUT_numpad_5x4(
		KC_NLCK, KC_PSLS, KC_PAST, KC_PMNS,
    	KC_P7,   KC_P8,   KC_P9, 
    	KC_P4,   KC_P5,   KC_P6, KC_PPLS, 
    	KC_P1,   KC_P2,   KC_P3, 
    	KC_P0,   KC_PDOT, KC_PENT
	),
};


void keyboard_pre_init_user(void) {
    // Call the keyboard pre init code.
    start_time = timer_read();

    // Set our LED pins as output (I don't have the pin sheet with me right now sorry carter lol)
    setPinInput(ENCODER_BUTTON);
}

void keyboard_post_init_user(void) {
    // Customise these values to desired behaviour
    debug_enable = true;
    debug_matrix = true;
    debug_keyboard = true;
    debug_mouse = true;

    rgblight_enable();
    rgblight_mode(RGBLIGHT_MODE_RAINBOW_SWIRL + 4);
}

void encoder_press_command(void) {
	switch (encoder_mode) {

        case 0: // Media control
			tap_code(KC_MEDIA_PLAY_PAUSE);
            break;

        case 1: // Volume control
			tap_code(KC_AUDIO_MUTE);
            break;

        case 2: // Backlight brightness control
			backlight_toggle();
            break;

        case 3: // RGB brightness control
			rgblight_toggle();
            break;

        case 4: // RGB speed control
			rgblight_toggle();
            break;

        case 5:
        	encoder_hue_sat = !encoder_hue_sat;
        	break;

        default: // Do nothing
        	break;
    }
}

void matrix_init_user(void) {

}

void matrix_scan_user(void) {
    numlock_status = layer_state_is(0);

    encoder_button_status = readPin(ENCODER_BUTTON);

    /* BUTTON PRESS LOOKS LIKE THIS:
                        falling          rising
    HIGH -----------------|                |---------------
                          |                |
    LOW                   |--------------- |
            not pressed        pressed        not pressed
    Change mode on falling edge of encoder press depending on duration of press */

    // Encoder is pressed
    if (!encoder_button_status) {
    	// Active on first scan after press
    	if (encoder_button_status_previous) {
	        encoder_start_time = timer_read();
	        encoder_button_status_previous = false;
    	}
    	// Active on all other press scans
    	else { 
    		// Check if hold duration is long enough
    		if (timer_read() - encoder_start_time > ENCODER_HOLD_DURATION) {
    			encoder_hold = true;
    		}
    		else {
    			encoder_hold = false;
    		}

    	}
    }
    // Encoder not pressed
    else {
    	// Active on first scan after depress
    	if (!encoder_button_status_previous) {
    	    // Check if holds active or else send normal press command
    		if (!encoder_hold && !encoder_hold_previous) {
    			encoder_press_command();
    		}

    		// Set previous bools
    		encoder_hold_previous = encoder_hold;
    		encoder_button_status_previous = true;
    	}
    }
}

static void erase_oled(void) {
    // Erase buffer
    oled_clear();

    // Render the empty buffer sequentially to the 16 OLED chunks
    for (uint8_t i = 0; i < 16; i++){
        oled_render();
    }
}

//Encoder stuff
//https://beta.docs.qmk.fm/using-qmk/hardware-features/feature_encoders
void encoder_update_user(uint8_t index, bool clockwise) {

	// If encoder was held, then change modes with rotation, else just change whatever mode encoder is on
	if (encoder_hold) { 
	    if (clockwise) {
	    	erase_oled();
            encoder_mode++;
        } else {
        	erase_oled();
            encoder_mode--;
        }
        if (encoder_mode > 5) {
        	encoder_mode = 0;
        }
        else if (encoder_mode < 0) {
        	encoder_mode = 5;
        }

	}
	else {
	    switch (encoder_mode) {

	        case 0: // Media control

	            if (clockwise) {
	                tap_code(KC_MEDIA_NEXT_TRACK);
	            } else {
	                tap_code(KC_MEDIA_PREV_TRACK);
	            }

	            break;

	        case 1: // Volume control

	            if (clockwise) {
	                tap_code(KC_VOLU);
	            } else {
	                tap_code(KC_VOLD);
	            }

	            break;

	        case 2: // Backlight brightness control

	            if (clockwise) {
	                backlight_increase();
	            } else {
	                backlight_decrease();
	            }

	            break;

	        case 3: // RGB brightness control

	            if (clockwise) {
	                rgblight_increase_val();
	            } else {
	                rgblight_decrease_val();
	            }

	            break;

	        case 4: // RGB mode control

	            if (clockwise) {
	                rgblight_step();
	            } else {
	                rgblight_step_reverse();
	            }

	            break;

	        case 5: // Change solidlight

	        	// Hue changing mode
	        	if (encoder_hue_sat) { 
		        	if (clockwise) {
		                rgblight_increase_hue();
		            } else {
		                rgblight_decrease_hue();
		            }
	        	}
	        	// Saturation changing mode
	        	else {
	        		if (clockwise) {
		                rgblight_increase_sat();
		            } else {
		                rgblight_decrease_sat();
		            }
	        	}

	        default: // Do nothing
	        	break;

	    }
	}
}

//OLED stuff
#ifdef OLED_DRIVER_ENABLE

// https://javl.github.io/image2cpp/
// Use 'Vertical - 1 bit per pixel' draw mode
// Make a logo now
static void render_logo(void) {
    static const char PROGMEM qmk_logo[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x9f, 0xcf, 0xef, 0xe7, 0xf7, 0xf3, 0xfb, 
		0xfb, 0xfb, 0xfb, 0xfb, 0xf3, 0xf7, 0xf7, 0xe7, 0xcf, 0x9f, 0x3f, 0x7f, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0x3f, 0x9f, 0xcf, 0xef, 0xe7, 0xf0, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xfd, 0xfc, 0xfe, 0xfe, 
		0xfe, 0xfc, 0xfd, 0xfd, 0xf9, 0xe3, 0x8f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xc1, 
		0xdd, 0xbd, 0xfd, 0xff, 0xc0, 0xff, 0xc7, 0x9b, 0xfb, 0xc7, 0xff, 0xc3, 0x9f, 0xdf, 0xc3, 0xef, 
		0xc3, 0xbb, 0xc0, 0xc0, 0xff, 0xc0, 0xfb, 0xd3, 0xc7, 0xe7, 0xc3, 0xbb, 0xd3, 0xc7, 0xcf, 0x8b, 
		0xc3, 0xc7, 0xff, 0xc3, 0xf3, 0xff, 0xc3, 0xbb, 0xc1, 0xc0, 0xf7, 0xa3, 0xcb, 0xdf, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xc0, 0x9f, 0x3f, 0x7f, 0x7f, 0x7f, 0xff, 0x07, 0x03, 0xfb, 0xfb, 0xfb, 0xfb, 0x7b, 
		0x7b, 0x9b, 0x0b, 0x8b, 0xfb, 0x7b, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 
		0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x8f, 0x9f, 0x7f, 0x0f, 0xff, 0x1f, 0x1f, 
		0x1f, 0x1f, 0xdf, 0x1f, 0x9f, 0x9f, 0x3f, 0xff, 0x07, 0xdf, 0x9f, 0x3f, 0xff, 0x1f, 0xff, 0x1f, 
		0x1f, 0xbf, 0x1f, 0x5f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xf0, 0xe0, 0xcf, 0xcf, 0xcf, 0xcc, 0xcc, 
		0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 
		0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xef, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfe, 0xfc, 0xff, 0xfe, 0xfe, 
		0xfe, 0xfe, 0xff, 0xfc, 0xff, 0xff, 0xfe, 0xff, 0xfe, 0xfc, 0xfc, 0xfe, 0xff, 0xfe, 0xfd, 0xfe, 
		0xfe, 0xff, 0xfd, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    oled_write_raw_P(qmk_logo, sizeof(qmk_logo));
}

void render_menu(void) {
    // static const char PROGMEM numlock_off[] = {
    //     0x00, 0x00, 0xfc, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0xe0, 0x00, 
    //     0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0xe0, 0x40, 0x20, 0x20, 0x20, 0xc0, 0x40, 0x20, 0x20, 
    //     0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x1f, 
    //     0x00, 0x00, 0x07, 0x08, 0x10, 0x10, 0x10, 0x08, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 
    //     0x1f, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00
    // };

    // static const char PROGMEM numlock_on[] = {
    //     0xfe, 0xff, 0x03, 0xf7, 0xef, 0xdf, 0xbf, 0x7f, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0x1f, 0xff, 
    //     0xff, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0x1f, 0xbf, 0xdf, 0xdf, 0xdf, 0x3f, 0xbf, 0xdf, 0xdf, 
    //     0xdf, 0x3f, 0xff, 0xfe, 0x3f, 0x7f, 0x60, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7d, 0x7b, 0x77, 0x60, 
    //     0x7f, 0x7f, 0x78, 0x77, 0x6f, 0x6f, 0x6f, 0x77, 0x60, 0x7f, 0x7f, 0x60, 0x7f, 0x7f, 0x7f, 0x7f, 
    //     0x60, 0x7f, 0x7f, 0x7f, 0x7f, 0x60, 0x7f, 0x3f
    // };

    // if (numlock_status) {
    //     oled_write_raw_P(numlock_on, sizeof(numlock_on));
    // }
    // else {
    //     oled_write_raw_P(numlock_off, sizeof(numlock_off));
    // }
    switch (encoder_mode) {

        case 0: // Media control
        	oled_set_cursor(0, 0);
        	oled_write("Encoder: Prev/Next", !encoder_hold);
        	oled_set_cursor(0, 1);
        	oled_write("Button : Play/Pause", !encoder_hold);
            break;

        case 1: // Volume control
        	oled_set_cursor(0, 0);
        	oled_write("Encoder: Volume", !encoder_hold);
        	oled_set_cursor(0, 1);
        	oled_write("Button : Mute", !encoder_hold);
            break;

        case 2: // Backlight brightness control
        	oled_set_cursor(0, 0);
        	oled_write("Encoder: LED Level", !encoder_hold);
        	oled_set_cursor(0, 1);
        	oled_write("Button : On/Off", !encoder_hold);
            break;

        case 3: // RGB brightness control
        	oled_set_cursor(0, 0);
        	oled_write("Encoder: RGB Level", !encoder_hold);
        	oled_set_cursor(0, 1);
        	oled_write("Button : On/Off", !encoder_hold);
            break;

        case 4: // RGB speed control
        	oled_set_cursor(0, 0);
        	oled_write("Encoder: RGB Mode", !encoder_hold);
        	oled_set_cursor(0, 1);
        	oled_write("Button : On/Off", !encoder_hold);
            break;

        case 5: // RGB speed control
        	oled_set_cursor(0, 0);
        	oled_write("Encoder: RGB Color", !encoder_hold);
        	oled_set_cursor(0, 1);
        	oled_write("Button : Hue/Sat", !encoder_hold);
            break;

        default: // Do nothing
        	break;
    }    
}


// static const char PROGMEM numlock_off[] = {
//   0x20, 0x20, 0x20, 0x20, 0x20, 0x99, 0x9A, 0x00
// };

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

void oled_task_user(void) {
    if (!boot_complete && timer_read() - start_time < BOOTTIME){
        render_logo();
    }
    else if (!boot_complete) {
        erase_oled();
        boot_complete = true;
    }
    else {
        render_menu();
    }
}
#endif

//DEBUG stuff to see if presses work and what not.
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
// If console is enabled, it will print the matrix position and status of each key pressed
#ifdef CONSOLE_ENABLE
    uprintf("KL: kc: %u, col: %u, row: %u, pressed: %u\n", keycode, record->event.key.col, record->event.key.row, record->event.pressed);
#endif
    return true;
}


void led_set_user(uint8_t usb_led) {

}
