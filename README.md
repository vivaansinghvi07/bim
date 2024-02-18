# stupid-text-editor
A text editor with the dumbest features imaginable.

## Design

- "Syntax" Highlighting: Highlights each word with a RGB value according to the first letter of the word. Punctuation all get the same highlighting.
- Modality: Like Vim, this editor will have multiple modes. However, many of these modes will be strange and very weird.
    - Substitution mode: write over whatever is currently there (I'm pretty sure Vim already has this)
    - Distinguish between uppercase and lowercase modes.
        - Optional: Make it impossible to type in the other case when in a certain case mode.
    - Have ASCII mode where each key correlates to its ASCII value.
    - File mode: Allow you to edit your directory where each line is a file, backspace goes to the parent dir, enter enters the file, etc
        - THIS IS VERY DANGEROUS. I CANNOT MESS THIS UP FOR THE SAKE OF MY SYSTEM.
- Exiting: You will not be able to exit this text editor.
