Qtmotion
========

Easymotion/leap.nvim/Jumpy/MetaJump-like plugin for QtCreator. Initially based on https://github.com/serpheroth/QtCreator-EasyMotion but heavily modified/re-written. 

It has two functions, jump before/after char and select from cursor to before/after char. To activate the former, press either `ctrl+,` or `ctrl+.`. To activate the latter, press either `ctrl+shift+,` or `ctrl+shift+.`. 

When activated it will show matching status in the top right of the text editor. An editor must have focus for it to work. Matching characters you can jump to will be shown in blue. Matching characters that require more input to jump to are shown in red. To reach these characters, you must type more characters that follow the desired destination position. If you make a mistake, you can press `Backspace`. You can exit the jump mode with `Esc`. 

Jump trigger characters are ordered by QWERTY finger travel distance and constrained by characters required for disambiguating following characters. They will spread out upward and downward from the current cursor position. If the cursor is not onscreen, it will start from the center of the screen. 

You can try the pre-built library plugins, or build from source with QtCreator. 

<p align="center">
  <img src="https://github.com/mp3guy/mp3guy.github.io/raw/master/img/Qtmotion.gif" alt="Qtmotion"/>
</p>
