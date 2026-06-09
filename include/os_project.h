#pragma once

#include <string>

void run_scheduler_module();
void run_memory_module();
void run_sync_module();
void run_filesystem_module();
void run_extension_module();

int read_int(const std::string &prompt, int min_value, int max_value);
void wait_for_enter();
