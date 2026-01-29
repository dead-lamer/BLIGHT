#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <linux/kernel.h>
#include "ascii_art.h"

#include <linux/module.h>


#define ACID_GREEN "\x1b[38;5;118m"
#define RESET "\x1b[0m"
#define CLEAR_SCREEN "\x1b[2J\x1b[H"

const char* banner[] = {
    "@@@@@@@   @@@@@@@@   @@@@@@   @@@@@@@",
    "@@@@@@@@  @@@@@@@@  @@@@@@@@  @@@@@@@@",
    "@@!  @@@  @@!       @@!  @@@  @@!  @@@",
    "!@!  @!@  !@!       !@!  @!@  !@!  @!@",
    "@!@  !@!  @!!!:!    @!@!@!@!  @!@  !@!",
    "!@!  !!!  !!!!!:    !!!@!!!!  !@!  !!!",
    "!!:  !!!  !!:       !!:  !!!  !!:  !!!",
    ":!:  !:!  :!:       :!:  !:!  :!:  !:!",
    " :::: ::   :: ::::  ::   :::   :::: ::",
    ":: :  :   : :: ::    :   : :  :: :  :",
    "",
    "@@@        @@@@@@   @@@@@@@@@@   @@@@@@@@  @@@@@@@",
    "@@@       @@@@@@@@  @@@@@@@@@@@  @@@@@@@@  @@@@@@@@",
    "@@!       @@!  @@@  @@! @@! @@!  @@!       @@!  @@@",
    "!@!       !@!  @!@  !@! !@! !@!  !@!       !@!  @!@",
    "@!!       @!@!@!@!  @!! !!@ @!@  @!!!:!    @!@!!@!",
    "!!!       !!!@!!!!  !@!   ! !@!  !!!!!:    !!@!@!",
    "!!:       !!:  !!!  !!:     !!:  !!:       !!: :!!",
    " :!:      :!:  !:!  :!:     :!:  :!:       :!:  !:!",
    " :: ::::  ::   :::  :::     ::    :: ::::  ::   :::",
    ": :: : :   :   : :   :      :    : :: ::    :   : :"
};

const int BANNER_HEIGHT = 21;
int BANNER_WIDTH;

typedef struct {
    int col;
    int source_row;
    int target_row;
    float current_row;
    float speed;
    char character;
    int settled;
    int phase;
} Drop;


/*
mode == 1: stdout
mode == 2: kernel_log
*/
void print_logo(int mode) {
    srand(time(NULL));
    
    switch (mode) {
        case 1:
            printf(CLEAR_SCREEN);
            fflush(stdout);
            break;
        case 2:
            break;
    }

    BANNER_WIDTH = 0;
    for (int i = 0; i < BANNER_HEIGHT; i++) {
        int len = strlen(banner[i]);
        if (len > BANNER_WIDTH) {
            BANNER_WIDTH = len;
        }
    }

    int first_layer_rows[] = {0, 1, 11, 12};
    int num_first_layer = 4;

    Drop* drops_phase2 = NULL;
    int num_drops_phase2 = 0;

    for (int i = 0; i < BANNER_HEIGHT; i++) {
        int is_first_layer = 0;
        for (int k = 0; k < num_first_layer; k++) {
            if (i == first_layer_rows[k]) {
                is_first_layer = 1;
                break;
            }
        }
        if (!is_first_layer) {
            int line_len = strlen(banner[i]);
            for (int j = 0; j < line_len; j++) {
                if (banner[i][j] == '@') {
                    num_drops_phase2++;
                }
            }
        }
    }

    drops_phase2 = malloc(num_drops_phase2 * sizeof(Drop));
    int idx2 = 0;

    for (int i = 0; i < BANNER_HEIGHT; i++) {
        int is_first_layer = 0;
        for (int k = 0; k < num_first_layer; k++) {
            if (i == first_layer_rows[k]) {
                is_first_layer = 1;
                break;
            }
        }
        if (!is_first_layer) {
            int line_len = strlen(banner[i]);
            for (int j = 0; j < line_len; j++) {
                if (banner[i][j] == '@') {
                    int source_row = -1;
                    for (int k = num_first_layer - 1; k >= 0; k--) {
                        if (first_layer_rows[k] < i) {
                            source_row = first_layer_rows[k];
                            break;
                        }
                    }

                    drops_phase2[idx2].col = j;
                    drops_phase2[idx2].source_row = source_row >= 0 ? source_row : 0;
                    drops_phase2[idx2].target_row = i;
                    drops_phase2[idx2].current_row = drops_phase2[idx2].source_row;
                    drops_phase2[idx2].speed = 0.15 + (rand() % 100) / 400.0;
                    drops_phase2[idx2].character = '@';
                    drops_phase2[idx2].settled = 0;
                    drops_phase2[idx2].phase = 2;
                    idx2++;
                }
            }
        }
    }

    Drop* drops_phase3 = NULL;
    int num_drops_phase3 = 0;

    for (int i = 0; i < BANNER_HEIGHT; i++) {
        int is_first_layer = 0;
        for (int k = 0; k < num_first_layer; k++) {
            if (i == first_layer_rows[k]) {
                is_first_layer = 1;
                break;
            }
        }
        if (!is_first_layer) {
            int line_len = strlen(banner[i]);
            for (int j = 0; j < line_len; j++) {
                if (banner[i][j] != ' ' && banner[i][j] != '@') {
                    num_drops_phase3++;
                }
            }
        }
    }

    drops_phase3 = malloc(num_drops_phase3 * sizeof(Drop));
    int idx3 = 0;

    for (int i = 0; i < BANNER_HEIGHT; i++) {
        int is_first_layer = 0;
        for (int k = 0; k < num_first_layer; k++) {
            if (i == first_layer_rows[k]) {
                is_first_layer = 1;
                break;
            }
        }
        if (!is_first_layer) {
            int line_len = strlen(banner[i]);
            for (int j = 0; j < line_len; j++) {
                if (banner[i][j] != ' ' && banner[i][j] != '@') {
                    int source_row = -1;
                    for (int search_row = i - 1; search_row >= 0; search_row--) {
                        if (banner[search_row][j] == '@') {
                            source_row = search_row;
                            break;
                        }
                    }

                    drops_phase3[idx3].col = j;
                    drops_phase3[idx3].source_row = source_row >= 0 ? source_row : i - 1;
                    drops_phase3[idx3].target_row = i;
                    drops_phase3[idx3].current_row = drops_phase3[idx3].source_row;
                    drops_phase3[idx3].speed = 0.2 + (rand() % 100) / 300.0;
                    drops_phase3[idx3].character = banner[i][j];
                    drops_phase3[idx3].settled = 0;
                    drops_phase3[idx3].phase = 3;
                    idx3++;
                }
            }
        }
    }

    char display[BANNER_HEIGHT + 15][BANNER_WIDTH + 1];

    Drop* puddle_drops = malloc(500 * sizeof(Drop));
    int num_puddle_drops = 0;
    int box_top = BANNER_HEIGHT + 3;
    int box_bottom = BANNER_HEIGHT + 10;
    int box_left = 0;
    int box_right = BANNER_WIDTH - 1;
    
    int box_top_line[BANNER_WIDTH];
    int box_bottom_line[BANNER_WIDTH];
    int box_left_line[15];
    int box_right_line[15];
    
    for (int i = 0; i < BANNER_WIDTH; i++) {
        box_top_line[i] = 0;
        box_bottom_line[i] = 0;
    }
    for (int i = 0; i < 15; i++) {
        box_left_line[i] = 0;
        box_right_line[i] = 0;
    }

    int frame = 0;
    int phase2_start = 0;
    int phase3_start = 0;
    int max_frames = 160; 
    int puddle_phase_start = 0; 
    int puddle_phase_end = 140; 

    int total_display_lines = BANNER_HEIGHT + 11 + 1; 

    while (frame < max_frames) {
        for (int i = 0; i < BANNER_HEIGHT + 15; i++) {
            memset(display[i], ' ', BANNER_WIDTH);
            display[i][BANNER_WIDTH] = '\0';
        }

        for (int k = 0; k < num_first_layer; k++) {
            int i = first_layer_rows[k];
            int line_len = strlen(banner[i]);
            for (int j = 0; j < line_len; j++) {
                display[i][j] = banner[i][j];
            }
        }

        if (frame >= phase2_start) {
            for (int d = 0; d < num_drops_phase2; d++) {
                if (!drops_phase2[d].settled) {
                    drops_phase2[d].current_row += drops_phase2[d].speed;

                    int row = (int)drops_phase2[d].current_row;

                    if (drops_phase2[d].current_row >= drops_phase2[d].target_row) {
                        display[drops_phase2[d].target_row][drops_phase2[d].col] = drops_phase2[d].character;
                        drops_phase2[d].settled = 1;
                    } else if (row >= 0 && row < BANNER_HEIGHT) {
                        if (display[row][drops_phase2[d].col] == ' ') {
                            display[row][drops_phase2[d].col] = ':';
                        }
                    }
                } else {
                    display[drops_phase2[d].target_row][drops_phase2[d].col] = drops_phase2[d].character;
                }
            }
        }

        if (frame >= phase3_start) {
            for (int d = 0; d < num_drops_phase3; d++) {
                if (!drops_phase3[d].settled) {
                    drops_phase3[d].current_row += drops_phase3[d].speed;

                    int row = (int)drops_phase3[d].current_row;

                    if (drops_phase3[d].current_row >= drops_phase3[d].target_row) {
                        display[drops_phase3[d].target_row][drops_phase3[d].col] = drops_phase3[d].character;
                        drops_phase3[d].settled = 1;
                    } else if (row >= 0 && row < BANNER_HEIGHT) {
                        if (display[row][drops_phase3[d].col] == ' ') {
                            display[row][drops_phase3[d].col] = '.';
                        }
                    }
                } else {
                    display[drops_phase3[d].target_row][drops_phase3[d].col] = drops_phase3[d].character;
                }
            }
        }

        if (frame >= puddle_phase_start && frame < puddle_phase_end) {
            if (frame % 8 == 0) {
                int source_rows[] = {9, 19};
                int source_row = source_rows[rand() % 2];

                int line_len = strlen(banner[source_row]);
                if (line_len > 0) {
                    int col = rand() % line_len;
                    if (col < BANNER_WIDTH && num_puddle_drops < 500) {
                        puddle_drops[num_puddle_drops].col = col;
                        puddle_drops[num_puddle_drops].current_row = source_row + 1;
                        puddle_drops[num_puddle_drops].target_row = box_top;
                        puddle_drops[num_puddle_drops].speed = 0.8 + (rand() % 100) / 100.0;
                        puddle_drops[num_puddle_drops].settled = 0;
                        puddle_drops[num_puddle_drops].character = '.';
                        num_puddle_drops++;
                    }
                }
            }
        }

        int all_puddle_settled = 1;
        for (int d = 0; d < num_puddle_drops; d++) {
            if (!puddle_drops[d].settled) {
                all_puddle_settled = 0;
                puddle_drops[d].current_row += puddle_drops[d].speed;

                int row = (int)puddle_drops[d].current_row;

                if (puddle_drops[d].current_row >= box_top) {
                    box_top_line[puddle_drops[d].col] = 1;
                    puddle_drops[d].settled = 1;
                } else if (row >= BANNER_HEIGHT && row < BANNER_HEIGHT + 15) {
                    if (display[row][puddle_drops[d].col] == ' ') {
                        display[row][puddle_drops[d].col] = ':';
                    }
                }
            }
        }
        
        int box_spread[BANNER_WIDTH];
        for (int i = 0; i < BANNER_WIDTH; i++) {
            box_spread[i] = box_top_line[i];
        }
        
        for (int col = 0; col < BANNER_WIDTH; col++) {
            if (box_top_line[col] == 1) {
                if (col > 0 && box_spread[col - 1] == 0 && rand() % 3 == 0) {
                    box_spread[col - 1] = 1;
                }
                if (col < BANNER_WIDTH - 1 && box_spread[col + 1] == 0 && rand() % 3 == 0) {
                    box_spread[col + 1] = 1;
                }
            }
        }
        
        for (int i = 0; i < BANNER_WIDTH; i++) {
            box_top_line[i] = box_spread[i];
        }
        

        if (box_top_line[0] == 1 || box_top_line[1] == 1) {
            for (int r = 0; r < box_bottom - box_top + 1; r++) {
                if (r == 0 || box_left_line[r - 1] == 1) {
                    box_left_line[r] = 1;
                }
            }
        }
        
        if (box_top_line[BANNER_WIDTH - 1] == 1 || box_top_line[BANNER_WIDTH - 2] == 1) {
            for (int r = 0; r < box_bottom - box_top + 1; r++) {
                if (r == 0 || box_right_line[r - 1] == 1) {
                    box_right_line[r] = 1;
                }
            }
        }
        
        int top_filled = 0;
        for (int i = 0; i < BANNER_WIDTH; i++) {
            if (box_top_line[i] == 1) top_filled++;
        }
        
        if (top_filled > BANNER_WIDTH / 2) {
            if (box_left_line[0] == 1) {
                box_bottom_line[0] = 1;
                box_bottom_line[1] = 1;
                box_bottom_line[2] = 1;
            }
            if (box_right_line[0] == 1) {
                box_bottom_line[BANNER_WIDTH - 1] = 1;
                box_bottom_line[BANNER_WIDTH - 2] = 1;
                box_bottom_line[BANNER_WIDTH - 3] = 1;
            }
            
            int bottom_spread[BANNER_WIDTH];
            for (int i = 0; i < BANNER_WIDTH; i++) {
                bottom_spread[i] = box_bottom_line[i];
            }
            
            for (int col = 0; col < BANNER_WIDTH; col++) {
                if (box_bottom_line[col] == 1) {
                    if (col > 0 && bottom_spread[col - 1] == 0 && rand() % 2 == 0) {
                        bottom_spread[col - 1] = 1;
                    }
                    if (col < BANNER_WIDTH - 1 && bottom_spread[col + 1] == 0 && rand() % 2 == 0) {
                        bottom_spread[col + 1] = 1;
                    }
                }
            }
            
            for (int i = 0; i < BANNER_WIDTH; i++) {
                box_bottom_line[i] = bottom_spread[i];
            }
        }

        int text_row1 = BANNER_HEIGHT + 5;
        int text_row2 = BANNER_HEIGHT + 7;
        
        const char* text1 = "Pentest Enthusiast, Script Kiddy";
        const char* text2 = "https://deadlamer.com";
        int text_len1 = strlen(text1);
        int text_len2 = strlen(text2);
        
        int start_col1 = (BANNER_WIDTH - text_len1) / 2;
        if (start_col1 < 2) start_col1 = 2;
        for (int j = 0; j < text_len1 && start_col1 + j < BANNER_WIDTH - 2; j++) {
            display[text_row1][start_col1 + j] = text1[j];
        }
        
        int start_col2 = (BANNER_WIDTH - text_len2) / 2;
        if (start_col2 < 2) start_col2 = 2;
        for (int j = 0; j < text_len2 && start_col2 + j < BANNER_WIDTH - 2; j++) {
            display[text_row2][start_col2 + j] = text2[j];
        }
        
        for (int col = 0; col < BANNER_WIDTH; col++) {
            if (box_top_line[col] == 1) {
                display[box_top][col] = '_';
            }
        }
        
        for (int r = box_top + 1; r < box_bottom; r++) {
            int rel_row = r - box_top;
            if (rel_row < 15) {
                if (box_left_line[rel_row] == 1) {
                    display[r][0] = '|';
                    if (BANNER_WIDTH > 1) display[r][1] = ' ';
                }
                if (box_right_line[rel_row] == 1) {
                    display[r][BANNER_WIDTH - 1] = '|';
                    if (BANNER_WIDTH > 2) display[r][BANNER_WIDTH - 2] = ' ';
                }
            }
        }
        
        for (int col = 0; col < BANNER_WIDTH; col++) {
            if (box_bottom_line[col] == 1) {
                display[box_bottom][col] = '_';
            }
        }
        
        if (box_top_line[0] == 1 && box_left_line[0] == 1) {
            display[box_top][0] = '_';
        }
        if (box_top_line[BANNER_WIDTH - 1] == 1 && box_right_line[0] == 1) {
            display[box_top][BANNER_WIDTH - 1] = '_';
        }

        if (frame > max_frames - 60 && all_puddle_settled) {
            for (int col = 0; col < BANNER_WIDTH; col++) {
                if (box_top_line[col] == 0 && rand() % 5 == 0) {
                    box_top_line[col] = 1;
                }
            }
            for (int r = 0; r < 15; r++) {
                if (box_left_line[r] == 0 && rand() % 6 == 0) {
                    box_left_line[r] = 1;
                }
                if (box_right_line[r] == 0 && rand() % 6 == 0) {
                    box_right_line[r] = 1;
                }
            }
            for (int col = 0; col < BANNER_WIDTH; col++) {
                if (box_bottom_line[col] == 0 && rand() % 5 == 0) {
                    box_bottom_line[col] = 1;
                }
            }
        }

        char full_output[8192];
        int offset = 0;

        offset += snprintf(full_output + offset, sizeof(full_output) - offset, "\x1b[%dA", total_display_lines);
        offset += snprintf(full_output + offset, sizeof(full_output) - offset, ACID_GREEN);

        for (int i = 0; i < BANNER_HEIGHT; i++) {
            offset += snprintf(full_output + offset, sizeof(full_output) - offset, "\r%-*s\x1b[K\n", BANNER_WIDTH, display[i]);
        }

        for (int i = BANNER_HEIGHT; i < BANNER_HEIGHT + 11; i++) {
            offset += snprintf(full_output + offset, sizeof(full_output) - offset, "\r%-*s\x1b[K\n", BANNER_WIDTH, display[i]);
        }
        
        
        offset += snprintf(full_output + offset, sizeof(full_output) - offset, RESET);

        switch(mode) {
            case 1:
                printf("%s", full_output);
                fflush(stdout);
                break;
            case 2:
                //printk(KERN_INFO "%s\n", full_output);
                break;
        }


        usleep(30000);
        frame++;
    }

    switch(mode) {
        case 1:
            printf("\n");
            break;
        case 2:
            //printk(KERN_INFO "\n");
            break;
    }

    free(drops_phase2);
    free(drops_phase3);
    free(puddle_drops);
}
