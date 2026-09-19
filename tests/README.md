# Ctrl+Backspace regression

The normal keyboard accelerator calls `CSendView::OnRepeatLastWord`.
If Ctrl+Backspace instead reaches `CSendView::OnChar` as DEL (`WM_CHAR`,
`0x7f`), the old implementation passes it to the underlying edit control.
Depending on that control's implementation, it can insert a control glyph
or ignore the character. Wine 11.17 ignores it.

The fix handles DEL using the existing command, including the world's
`ctrl_backspace_deletes_last_word` option. The option remains off by default
for compatibility: off recalls the last word of the previous command; on
deletes the word before the caret. Normal accelerator handling, including
plugin bindings, is unchanged.

## Running the integration test in Wine

1. Build MUSHclient with Visual Studio 2022 / v143 and MFC, or download the
   `MUSHclient_Release` artifact from this fork's GitHub Actions build.
2. Install the official MUSHclient distribution in a disposable Wine prefix
   to obtain `lua5.1.dll`, `locale/en.dll`, and the other runtime resources.
   Replace its executable with the build under test.
3. Open `tests/ctrl-backspace-test.mcl` in that MUSHclient. It does not
   automatically connect to a server. Dismiss any startup dialogs.
4. Compile and run the test with the same `WINEPREFIX`:

   ```sh
   winegcc -m64 tests/ctrl_backspace.c -o /tmp/ctrl_backspace.exe
   WINEDEBUG=-all /tmp/ctrl_backspace.exe
   ```

The test locates only the supplied test world's command input and replaces
its contents. It checks word deletion, trailing spaces, deletion in the
middle of text, selections, empty input, the start of input, undo, and plain
Backspace. It also checks the existing accelerator command and the legacy
word-recall option. It sends `WM_CHAR` deliberately, so it tests the fallback
rather than relying on keyboard acceleration. It does not inject desktop
keystrokes. The test world needs Lua scripting enabled (as in the fixture).

The original executable fails seven cases on Wine 11.17. The fixed executable
must pass all twelve checks. This reproduces the unhandled character path; it
does not claim that every keyboard layout or Wine version produces that
path from a physical key press.

On Windows, compile in a Visual Studio developer prompt with
`cl tests\ctrl_backspace.c user32.lib`, open the same world, and run
`ctrl_backspace.exe`.
