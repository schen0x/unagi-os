/**
 * @file logger.hpp
 *
 * カーネルロガーの実装．
 */

#pragma once

enum LogLevel
{
  kError = 3,
  kWarn = 4,
  kInfo = 6,
  kDebug = 7,
};

/** @brief Set global LogLevel
 * - subsequent Log(...) only write when level < LogLevel (lower is more critical)
 *
 * グローバルなログ優先度のしきい値を level に設定する．
 * 以降の Log の呼び出しでは，ここで設定した優先度以上のログのみ記録される．
 */
void SetLogLevel(LogLevel level);

/** @brief Logging with level
 * - Log(...) only write when level < LogLevel (lower is more critical)
 * - default global LogLevel is kWarn
 *
 * ログを指定された優先度で記録する．
 *
 * 指定された優先度がしきい値以上ならば記録する．
 * 優先度がしきい値未満ならログは捨てられる．
 *
 * @param level  ログの優先度．しきい値以上の優先度のログのみが記録される．
 * @param format  書式文字列．printk と互換．
 */
int Log(LogLevel level, const char *format, ...);
/**
 * Usage:
 * Call this, and set breakpoint in debugger on this function
 */
int debug_break();
