#ifndef DJ_DEBUG_H
#define DJ_DEBUG_H

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

// Simple file-based logger for immediate user feedback
// path: djcc.txt (current directory)
// Global flag to control logging (defined in GameLogic.cpp)
extern bool g_enableDjLog;

// Maximum lines to write (prevents disk overflow)
#define DJ_LOG_MAX_LINES 100000
extern int g_djLogLineCount; // Defined in GameLogic.cpp
extern bool g_djLogReset;    // Defined in GameLogic.cpp or similar

static void DjLog(const char *format, ...) {
  if (!g_enableDjLog)
    return;

  // One-time log clearing
  if (!g_djLogReset) {
    const char *path = "d:\\djcc.txt";
    FILE *f = fopen(path, "w");
    if (f) {
      // Reset file is empty now
      fclose(f);
    }
    g_djLogLineCount = 0;
    g_djLogReset = true;
  }

  if (g_djLogLineCount >= DJ_LOG_MAX_LINES)
    return;

  // MessageBox(NULL, "DjLog called", "Debug", MB_OK); // Too noisy for normal
  // use, but good for --test

  char buffer[2048];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, 2048, format, args);
  va_end(args);

  // Send to Debugger (DebugView)
  OutputDebugString(buffer);
  OutputDebugString("\n");

  // Try writing to multiple paths
  const char *path = "d:\\djcc.txt";

  FILE *f = fopen(path, "a");

  if (!f) {
    // Diagnostic: why did it fail?
    char errBuf[256];
    snprintf(errBuf, 256, "DjLog: Failed to open %s\n", path);
    OutputDebugString(errBuf);
  }

  if (!f) {
    // No MessageBox here to avoid spamming if it works.
  }

  if (f) {
    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) {
      fprintf(f, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
    }
    fprintf(f, "%s\n", buffer);
    fflush(f); // Force write to disk immediately
    fclose(f);
    g_djLogLineCount++;

    // Log when limit reached
    if (g_djLogLineCount >= DJ_LOG_MAX_LINES) {
      FILE *fLimit = fopen(path, "a");
      if (fLimit) {
        fprintf(fLimit,
                "\n### LOG LIMIT REACHED (%d lines) - LOGGING STOPPED ###\n",
                DJ_LOG_MAX_LINES);
        fclose(fLimit);
      }
    }
  }
}

// Call this to reset log counter (e.g., on new game)
// Call this to reset log counter (e.g., on reset)
static void DjLog_Clear() {
  // Reset logs
  // Reset logs
  const char *path = "d:\\djcc.txt";
  FILE *f = fopen(path, "w");
  if (f) {
    fflush(f);
    fclose(f);
  }
  g_djLogLineCount = 0;

  // Log CWD
  char cwd[1024];
  if (GetCurrentDirectory(1024, cwd)) {
    DjLog("CWD: %s", cwd);
  } else {
    DjLog("CWD: Unknown");
  }
}

#endif
