# bim

I was originally going to make this have extrememly dumb features, but I ended up wanting
to implement a lot of the features of vim. 

## Installation

Simply run `$ make` to build the editor in the current directory. To install it to `usr/local/bin/`, run `$ make install`.

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
| `text_style` | `BOLD`, `NORMAL`, `ITALIC` | Sets the style of the text displayed |
| `highlight_mode` | `GRADIENT`, `LEXICAL`, `RGB`, `RANDOM`, `NONE` | Sets the way in which text is highlighted |
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

Like `vim`, this is a modal editor. Currently, there are three main modes and several "command" modes.
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
| `w`/`s` |  `normal`, `files`  | Moves the cursor up/down |
| `A-U`/`A-D` |  `normal`, `files`, `edit`  | Moves the cursor up/down |
| `a`/`d` |  `normal`  | Moves the cursor left/right |
| `A-L`/`A-R` |  `normal`, `edit`  | Moves the cursor left/right |
| `C-w`/`C-a`/`C-s`/`C-d`/ | `edit`| Moves the cursor up/left/down/right |
| `W`/`S` |  `normal`, `files` | If in the middle of a buffer, moves the cursor up/down half the screen height. Otherwise, if the start/end of the buffer is visible on the screen, jumps there. |
| `A`/`D` |  `normal`, `files` | If in the middle of a line, moves the cursor left/right half the screen width. Otherwise, if the start/end of the line is visible on the screen, jumps there. |

## Design

These are ideas and may or may not end up being implemented. If this message is displayed, this editor is still under development.

- "Syntax" Highlighting: Highlights each word with a RGB value according to the first letter of the word. Punctuation all get the same highlighting.
- Modality: Like Vim, this editor will have multiple modes. However, many of these modes will be strange and very weird.
    - Substitution mode: write over whatever is currently there (I'm pretty sure Vim already has this)
    - Distinguish between uppercase and lowercase modes.
        - Optional: Make it impossible to type in the other case when in a certain case mode.
    - Have ASCII mode where each key correlates to its ASCII value.
    - File mode: Allow you to edit your directory where each line is a file, backspace goes to the parent dir, enter enters the file, etc
        - THIS IS VERY DANGEROUS. I CANNOT MESS THIS UP FOR THE SAKE OF MY SYSTEM.
- Exiting: The only way to exit this text editor is to remain inactive for more than ten seconds. 
    - After five seconds, your code falls to the ground like in the [Cellular Automata Neovim plugin](https://github.com/Eandrju/cellular-automaton.nvim) plugin.
- Similar to Vim, you can move using given keys; however, which keys do this is yet to be decided. I can either make it stupid or functional.
- For syntax highlighting in the gradient mode, allow the user to press a key to rotate the direction of the gradient by 45 degress counter/clockwise.
