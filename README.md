# bim

Yet another text editor! I was originally going to make this have extrememly dumb features, but I ended up wanting
to implement a lot of the features of `vim`. Hence the name, bum `vim`, or `bim`. Additionally, I challenged myself to
create this without using any third-party dependencies!

https://github.com/vivaansinghvi07/bim/assets/122464874/ee71cd21-af5b-4e97-a15f-39c790764696

## Installation

Simply run `$ make` to build the editor in the current directory. To install it to `usr/local/bin/`, run `$ make install`.

## Limitations & Potential Improvements

- There are some memory leaks that I will eventually look for and patch.
        - Current ones I know about include in the `handle_c_search_word` function, and a little in displaying the buffer.
- This may or may not work on Windows, as I've only tested it on my Macbook.
- There is no support for Unicode or other text encodings. I may consider adding support for them in the future, though.
- There is no LSP support. However, I am considering learning some basic LSP protocol to get something working.
        - Reference: [neovim](https://github.com/neovim/neovim/blob/master/runtime/lua/vim/lsp/client.lua] - search for `initialize`
- The non-256-color terminal support is only limited to non-256-color terminals, and if it is enabled in a 256-color terminal, the colors do not work properly.
  This is because I was too lazy to change the ANSI format that styles are printed with.

## Usage

### Configuration

All configuration for `bim` is done in the `.bim_rc` file in your home directory. Each line that is a 
configuration option must contain the option immediately followed by an equals sign, which is then immediately followed by the value.

Here is an example configuration:

```
highlight_mode=RGB
tab_width=8

screensaver_mode=ROCK_PAPER_SCISSORS 
screensaver_ms_inactive=-1

gradient_cycle_duration_ms=100

rgb_cycle_duration_ms=20
rgb_angle=315
```

Below is a table of all configuration options and their usages.

| Option | Values | Description |
| :----- | :----: | :---------- |
| `supports_256_color` | `TRUE`, `FALSE`, `Y`, `N` | Sets if the terminal has 256-color support. This should not be set false in a 256-color terminal. |
| `text_style` | `BOLD`, `NORMAL`, `ITALIC` | Sets the style of the text displayed |
| `highlight_mode` | `SYNTAX`, `GRADIENT`, `LEXICAL`, `RGB`, `RANDOM`, `NONE` | Sets the way in which text is highlighted |
| `gradient_left` | An RGB color in hex notation (with the hashtag) | Sets the left color of the gradient when `highlight_mode` is GRADIENT |
| `gradient_right` | An RGB color in hex notation (with the hashtag) | Sets the right color of the gradient when `highlight_mode` is GRADIENT |
| `screensaver_mode` | `LEFT_SLIDE`, `RIGHT_SLIDE`, `TOP_SLIDE`, `BOTTOM_SLIDE`, `ROCK_PAPER_SCISSORS`, `FALLING_SAND`, `GAME_OF_LIFE` | Sets the screensaver to use when inactive |
| `gradient_angle` | `0`, `45`, `90`, `135`, `180`, `225`, `270`, `315` | Sets the angle of tilt for the gradient when `highlight_mode` is GRADIENT |
| `rgb_angle` | `0`, `45`, `90`, `135`, `180`, `225`, `270`, `315` | Sets the angle of tilt for the RGB pattern when `highlight_mode` is RGB |
| `screensaver_ms_inactive` | A whole number | Sets the number of milliseconds to be inactive until the screensaver starts playing |
| `screensaver_frame_length_ms` | A whole number | Sets the frame length of each tick of the screensaver |
| `gradient_cycle_duration_ms` | A whole number | Sets the frame length of each tick of the rotating animation when `highlight_mode` is GRADIENT |
| `rgb_cycle_duration_ms` | A whole number | Sets the frame length of each tick of the RGB animation when `highlight_mode` is RGB |
| `tab_width` | A whole number | Sets the tab width of the editor (tabs are automatically converted to spaces) |

### Modes

Like `vim`, this is a modal editor. Currently, there are three main modes and several 'command' modes.
Each command mode shares an input handler, but has different effects when the command is entered.

|     Mode     | Description |
| :----------: | :---------- |
| `normal`     | Pretty much like `vim`'s normal mode, this mode allows fast navigations and features many keybindings to make text editing faster. A list of these bindings is in the next table. This mode is activated by default when the editor is open and can be accessed using the escape key. |
| `files`      | Allows for a manipulation of your file tree through the editor. Access this mode with the `f` keybinding from `normal` mode. Like normal mode, many bindings are featured to make navigation and file manipulation easy. |
| `edit`       | A pure edit mode, taking all printable characters (defined by `isprint()` in C) and using them to modify the current buffer. You can exit this using the escape key. |

The rest of these are `command` modes, which all create a prompt for the user at the bottom of the editor. The user then types in
the response using printable characters. These can be exited with the escape key, or submitted with the enter key.

| &nbsp;&nbsp;Command&nbsp;Name&nbsp;&nbsp; | Description |
| :----------: | :---------- |
| `search`     | Searches for a string in the buffer, and can be used in forwards or backwards order. Access this mode using `;` for a forwards search and `:` for a backwards search. |
| `file search`| The variation of the above command that works in `files` mode. It is called with the same keys and also features forwards and backwards searching. |
| `rename`     | Renames the file that the user was hovering over in `files` mode. |
| `confirm delete` | A confirmation for deleting the file that the user was hovering over in `files` mode. |
| `open` | Opens a file using either relative or absolute paths, toggled from `normal` mode. |
| `create file` | Creates a file in `files` mode, in the current directory of the mode. |

### Keybindings

Like `vim`, `bim` also features plenty of useful keybindings with their own functions in the editor.

|   Key   |  Mode(s)   |  Description |
| -----:  |  :-----:   |  :---------- |
| `w`/`s` |  `normal`, `files`  | Moves the cursor up/down. |
| `A-U`/`A-D` |  `normal`, `files`, `edit`  | Moves the cursor up/down. |
| `a`/`d` |  `normal`  | Moves the cursor left/right. |
| `A-L`/`A-R` |  `normal`, `edit`  | Moves the cursor left/right. |
| `C-w`/`C-a`/`C-s`/`C-d` | `edit`| Moves the cursor up/left/down/right. |
| `W`/`S` |  `normal`, `files` | If in the middle of a buffer, moves the cursor up/down half the screen height. Otherwise, if the start/end of the buffer is visible on the screen, jumps there. |
| `A`/`D` |  `normal`, `files` | If in the middle of a line, moves the cursor left/right half the screen width. Otherwise, if the start/end of the line is visible on the screen, jumps there. |
| `?` | `normal` | Shuffles the color theme when `highlight_mode` is `LEXICAL` or `SYNTAX`. |
| `!` | `normal` | Reloads the configuration settings (useful when editing the `.bim_rc` file within `bim` to see changes instantly). |
| `+` | `normal` | Increments the angle of the display when `highlight_mode` is `RGB` or `GRADIENT`. |
| `-` | `normal` | Decrements the angle of the display when `highlight_mode` is `RGB` or `GRADIENT`. |
| `]` | `normal` | Goes to the next open buffer. |
| `[` | `normal` | Goes to the previous open buffer. |
| `e` | `normal` | Enters `edit` mode. |
| `z` | `normal` | Saves the current buffer. |
| `Z` | `normal` | Saves all open buffers. |
| `R` | `normal` | Removes a line and writes it to the copy register. |
| `r` | `normal` | Removes a character and writes it to the copy register. When this and the above are used with a number beforehand, all the lines/characters are added to the copy register. |
| `C` | `normal` | Writes a line to the copy register. |
| `c` | `normal` | Writes a character to the copy register. These two are also affected by a preceding number. |
| `P` | `normal` | Pastes the copy register onto a newline below the current one. |
| `p` | `normal` | Pastes the copy register at the location of the cursor. |
| `l` | `normal` | Jumps to the line specified by the number entered beforehand. If there is no number, jumps to the top of the buffer. |
| `;` | `normal` | Enters `search` mode, searching forwards. |
| `;` | `files`  | Enters `file search` mode, searching forwards. |
| `:` | `normal` | Enters `search` mode, searching backwards. |
| `:` | `files`  | Enters `file search` mode, searching backwards. |
| `j` | `normal`, `files` | Jumps to the next match (of the phrase entered in `search` or `file search` mode). If searching backwards, the direction is backwards. |
| `J` | `normal`, `files` | Jumps to the previous match. If searching backwards, the direction is forwards. |
| `>` | `normal` | Shifts the current line forwards by one tab. |
| `<` | `normal` | Shifts the current line backwards by one tab. |
| `<` | `files` | Navigates to the parent directory. |
| `M` | `normal`, `files` | Starts recording a macro. If already recording a macro, ends recording the macro. |
| `m` | `normal`, `files` | Plays a macro. This cannot be used while recording a macro. |
| `o` | `normal` | Enters `open` mode. |
| `o` | `files`  | Enters `create file` mode. |
| `Bksp` | `files` | Enters `confirm delete` mode, which deletes the file under the cursor if prompted "y". |
| `f` | `normal` | Enters `files` mode. |
| `n` | `normal` | Jumps to the next word, which can be either a name (defined as matching `[A-Za-z0-9_#$]+`) or a combination of symbols. |
| `N` | `normal` | Jumps to the previous word. |
| `b` | `normal` | Deletes the word under the cursor, including succeeding whitespace. |
| `B` | `normal` | Deletes the text in a line from the cursor to the end. |
| `v` | `normal` | Searches for the next occurance of the token under the cursor. |
| `V` | `normal` | Searches for the previous occurance of the token under the cursor. |
| `Esc` | any mode | Returns to `normal` mode. If is done in a `command` mode, discards any inputted text. |
| `Enter` | `command` mode | Submits the inputted text to the command. |
