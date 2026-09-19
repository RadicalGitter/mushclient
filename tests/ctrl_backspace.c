/* Integration test against a running MUSHclient command edit control.
 * Build: winegcc -m64 tests/ctrl_backspace.c -o /tmp/ctrl_backspace.exe
 * Or: cl tests\ctrl_backspace.c user32.lib
 * Usage: ctrl_backspace.exe [command-edit-HWND-in-hex]
 * With no argument, finds the supplied ctrl-backspace-test.mcl world.
 * This exercises WM_CHAR directly, the fallback path missed by accelerators.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static HWND test_edit;

static BOOL CALLBACK find_edit(HWND window, LPARAM unused)
{
  char cls[64], title[256];
  (void)unused;
  GetClassNameA(window, cls, sizeof cls);
  if (strcmp(cls, "Edit") || GetDlgCtrlID(window) != 59664) return TRUE;
  GetWindowTextA(GetParent(GetParent(window)), title, sizeof title);
  if (strstr(title, "ctrl-backspace-test.mcl")) test_edit = window;
  return TRUE;
}

static BOOL CALLBACK find_world(HWND window, LPARAM unused)
{
  (void)unused;
  EnumChildWindows(window, find_edit, 0);
  return TRUE;
}

static void script(HWND edit, const char *command)
{
  SendMessageA(edit, WM_SETTEXT, 0, (LPARAM)command);
  SendMessageA(edit, WM_CHAR, VK_RETURN, 1);
}

static void check(HWND edit, const char *name, const char *input,
                  int start, int end, const char *expected, int caret)
{
  char actual[1024];
  DWORD sel_start, sel_end;
  SendMessageA(edit, WM_SETTEXT, 0, (LPARAM)input);
  SendMessageA(edit, EM_SETSEL, start, end);
  SendMessageA(edit, WM_CHAR, 0x7f, 1);
  SendMessageA(edit, WM_GETTEXT, sizeof actual, (LPARAM)actual);
  SendMessageA(edit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
  if (strcmp(actual, expected) || sel_start != (DWORD)caret || sel_end != (DWORD)caret)
    {
    printf("FAIL %s: text=[%s], selection=%lu,%lu\n", name, actual,
           (unsigned long)sel_start, (unsigned long)sel_end);
    failures++;
    }
  else
    printf("PASS %s\n", name);
}

int main(int argc, char **argv)
{
  HWND edit;
  char cls[64], actual[1024];
  if (argc > 2) { puts("Usage: ctrl_backspace.exe [edit-HWND-in-hex]"); return 2; }
  if (argc == 2)
    edit = (HWND)(ULONG_PTR)strtoull(argv[1], NULL, 16);
  else
    {
    EnumWindows(find_world, 0);
    edit = test_edit;
    }
  if (!IsWindow(edit) || !GetClassNameA(edit, cls, sizeof cls) || strcmp(cls, "Edit") ||
      GetDlgCtrlID(edit) != 59664)
    { puts("Open tests/ctrl-backspace-test.mcl in MUSHclient first"); return 2; }

  check(edit, "previous word", "one two three", 13, 13, "one two", 7);
  check(edit, "only word", "hello", 5, 5, "", 0);
  check(edit, "trailing spaces", "one two   ", 10, 10, "one", 3);
  check(edit, "middle of command", "one two three", 7, 7, "one three", 3);
  check(edit, "selection", "one two three", 4, 8, "one three", 4);
  check(edit, "start of command", "one two", 0, 0, "one two", 0);
  check(edit, "empty command", "", 0, 0, "", 0);
  check(edit, "undo preparation", "one two", 7, 7, "one", 3);
  SendMessageA(edit, EM_UNDO, 0, 0);
  SendMessageA(edit, WM_GETTEXT, sizeof actual, (LPARAM)actual);
  if (strcmp(actual, "one two")) { puts("FAIL undo"); failures++; }
  else puts("PASS undo");

  SendMessageA(edit, WM_SETTEXT, 0, (LPARAM)"ab");
  SendMessageA(edit, EM_SETSEL, 2, 2);
  SendMessageA(edit, WM_CHAR, VK_BACK, 1);
  SendMessageA(edit, WM_GETTEXT, sizeof actual, (LPARAM)actual);
  if (strcmp(actual, "a")) { puts("FAIL plain backspace"); failures++; }
  else puts("PASS plain backspace");

  SendMessageA(edit, WM_SETTEXT, 0, (LPARAM)"one two three");
  SendMessageA(edit, EM_SETSEL, 13, 13);
  /* ID_REPEAT_LAST_WORD: the same command dispatched by the accelerator. */
  SendMessageA(GetAncestor(edit, GA_ROOT), WM_COMMAND, MAKEWPARAM(32973, 1), 0);
  SendMessageA(edit, WM_GETTEXT, sizeof actual, (LPARAM)actual);
  if (strcmp(actual, "one two")) { puts("FAIL accelerator command"); failures++; }
  else puts("PASS accelerator command");

  script(edit, "/SetOption('ctrl_backspace_deletes_last_word', 0)");
  script(edit, "/-- remembered");
  check(edit, "legacy word recall", "say ", 4, 4, "say remembered", 14);
  script(edit, "/SetOption('ctrl_backspace_deletes_last_word', 1)");
  SendMessageA(edit, WM_SETTEXT, 0, (LPARAM)"");
  printf("%d failure(s)\n", failures);
  return failures ? 1 : 0;
}
