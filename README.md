# stupid-text-editor
A text editor with the dumbest features imaginable.

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
