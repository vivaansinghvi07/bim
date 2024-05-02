#include "input/normal.h"
#include "input/command.h"
#include "state.h"
#include "utils.h"
#include "mode.h"

static const editor_mode_t MODES[] = {
        { 
                .type = NORMAL, .title_text = "normal", .title_len = 6, .input_handler = handle_normal_input,
                .escape_sequence_handler = handle_normal_escape_sequence_input, .displays_files = false, .track_input = true,
                .command_destination = NORMAL, .command_enter_handler = NULL
        },
        {
                .type = FILES, .title_text = "files", .title_len = 5, .input_handler = handle_files_input,
                .escape_sequence_handler = handle_files_escape_sequence_input, .displays_files = true, .track_input = true,
                .command_destination = FILES, .command_enter_handler = NULL
        },
        {
                .type = EDIT, .title_text = "edit", .title_len = 4, .input_handler = handle_edit_input,
                .escape_sequence_handler = handle_edit_escape_sequence_input, .displays_files = false, .track_input = false,
                .command_destination = EDIT, .command_enter_handler = NULL
        },
        {
                .type = CMD_SEARCH, .title_text = "search", .title_len = 6, .input_handler = handle_command_input,
                .escape_sequence_handler = NULL, .displays_files = false, .track_input = false,
                .command_destination = NORMAL, .command_enter_handler = handle_search_command
        },
        {
                .type = CMD_OPEN, .title_text = "open", .title_len = 4, .input_handler = handle_command_input,
                .escape_sequence_handler = NULL, .displays_files = false, .track_input = false,
                .command_destination = NORMAL, .command_enter_handler = handle_open_command
        },
        {
                .type = CMD_RENAME, .title_text = "rename", .title_len = 6, .input_handler = handle_command_input,
                .escape_sequence_handler = NULL, .displays_files = true, .track_input = false, 
                .command_destination = FILES, .command_enter_handler = handle_rename_command
        }
};

void check_mode_array(void) {
        for (size_t i = 0; i < EDITOR_MODE_TYPE_COUNT; ++i) {
                if (i != MODES[i].type) {
                        exit_error("Fatal: Invalid mode detected in mode array. Mode enum must match index.\n");
                }
        }
}

// this is a very simple function but it is one in case i change this
const editor_mode_t *mode_from(editor_mode_type_t mode) {
        return MODES + mode;
}